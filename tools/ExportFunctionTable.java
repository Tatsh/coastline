// Prints one tab-separated row per function of the current program, for tools/progress.py.
// The columns are qualified name, entry address, body length, reference count, thunk flag,
// external flag, and prototype.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.ReferenceManager;

public class ExportFunctionTable extends GhidraScript {
  @Override
  public void run() throws Exception {
    ReferenceManager references = currentProgram.getReferenceManager();
    FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
    while (functions.hasNext()) {
      Function function = functions.next();
      println(String.join("\t", function.getName(true), function.getEntryPoint().toString(),
          Long.toString(function.getBody().getNumAddresses()),
          Integer.toString(references.getReferenceCountTo(function.getEntryPoint())),
          Boolean.toString(function.isThunk()), Boolean.toString(function.isExternal()),
          function.getPrototypeString(false, false)));
    }
  }
}
