import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;

// Renames or deletes symbols by name. The arguments are <apply 0|1> and <old:new;old:new;...>,
// where a new name of '-' deletes the label. Without apply, it only lists what it would change.
public class SyncNames extends GhidraScript {
  public void run() throws Exception {
    boolean apply = getScriptArgs()[0].equals("1");
    SymbolTable st = currentProgram.getSymbolTable();
    FunctionManager fm = currentProgram.getFunctionManager();
    for (String pair : getScriptArgs()[1].split(";")) {
      String[] f = pair.split(":");
      List<Symbol> syms = new ArrayList<>();
      SymbolIterator it = st.getSymbols(f[0]);
      while (it.hasNext()) syms.add(it.next());
      for (Symbol s : syms) {
        Function fn = fm.getFunctionAt(s.getAddress());
        if (f[1].equals("-")) {
          println(String.format("DEL %s %s %s", f[0], s.getAddress(), s.getSymbolType()));
          if (apply && fn == null) s.delete();
        } else {
          String size = fn != null ? String.format("0x%X", fn.getBody().getNumAddresses()) : "-";
          println(String.format("REN %s %s %s %s size=%s", f[0], f[1], s.getAddress(),
              s.getSymbolType(), size));
          if (apply) s.setName(f[1], SourceType.USER_DEFINED);
        }
      }
    }
  }
}
