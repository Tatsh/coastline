import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

// Writes a splat symbol_addrs file for the loaded image. The arguments are <out> and <bss end hex>.
public class SymbolsExport extends GhidraScript {
  String fix(String name) {
    return name.replaceAll("[^A-Za-z0-9_]", "_");
  }

  public void run() throws Exception {
    long bssEnd = Long.decode(getScriptArgs()[1]);
    Listing listing = currentProgram.getListing();
    TreeMap<Long, String> lines = new TreeMap<>();
    Set<String> used = new HashSet<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      long a = f.getEntryPoint().getOffset();
      if (a < 0x80200000L || a >= bssEnd) continue;
      String n = fix(f.getName());
      used.add(n);
      lines.put(a, String.format("%s = 0x%08X; // type:func size:0x%X", n, a,
          f.getBody().getMaxAddress().getOffset() + 1 - a));
    }
    for (Symbol s : currentProgram.getSymbolTable().getAllSymbols(true)) {
      long a = s.getAddress().getOffset();
      if (a < 0x80200000L || a >= bssEnd || lines.containsKey(a)) continue;
      if (s.getSymbolType() != SymbolType.LABEL || !s.isPrimary()) continue;
      // Skip a name Ghidra generated from the data type, such as BYTE_80380672.
      if (s.getSource() == SourceType.DEFAULT) continue;
      if (listing.getInstructionContaining(s.getAddress()) != null) continue;
      String n = fix(s.getName());
      if (used.contains(n)) n = n + "_" + Long.toHexString(a);
      used.add(n);
      Data d = listing.getDataAt(s.getAddress());
      String extra = "";
      if (d != null && d.getLength() > 1 && !(d.getDataType() instanceof Undefined)) {
        extra = String.format(" size:0x%X", d.getLength());
        if (d.getDataType() instanceof StringDataType || d.getDataType().getName().equals("string")) {
          extra += " type:asciz";
        }
      }
      lines.put(a, String.format("%s = 0x%08X; //%s", n, a, extra.isEmpty() ? "" : extra));
    }
    try (PrintWriter w = new PrintWriter(new FileWriter(getScriptArgs()[0]))) {
      for (String l : lines.values()) w.println(l);
    }
    println("symbols " + lines.size());
  }
}
