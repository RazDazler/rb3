// Export selected local Ghidra drafts and call references. Never edit source.
// @author ChatGPT
// @category RockBand3
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.lang.InjectPayload;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.data.VoidDataType;
import ghidra.xml.XmlPullParserFactory;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.charset.StandardCharsets;

public class ExportB8Context extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("request.tsv output-directory required");
        Path output = Paths.get(args[1]);
        Files.createDirectories(output);
        // These compiler helpers only preserve the caller's nonvolatile registers
        // on its stack frame. Treat their bookkeeping as a register-preserving
        // no-op for draft generation, avoiding a spurious return value/clobber.
        // The read-only project discards these analysis annotations after export.
        String fixup = "rb3_abi_frame_helper";
        String xml = "<callfixup name=\"" + fixup + "\"><pcode><body><![CDATA[r3 = r3;]]></body></pcode></callfixup>";
        currentProgram.getCompilerSpec().getPcodeInjectLibrary().restoreXmlInject(
            "B8 compiler ABI annotation", fixup, InjectPayload.CALLFIXUP_TYPE,
            XmlPullParserFactory.create(xml, "B8 compiler ABI annotation", null, false));
        for (Function helper : currentProgram.getFunctionManager().getFunctions(true)) {
            if (helper.getName().matches("_(save|rest)(gpr|fpr)_(1[4-9]|2[0-9]|3[01])")) {
                helper.setReturnType(VoidDataType.dataType, SourceType.USER_DEFINED);
                helper.setCallFixup(fixup);
            }
        }
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        StringBuilder index = new StringBuilder("address\tsymbol\tcompleted\tartifact\n");
        try {
            for (String row : Files.readAllLines(Paths.get(args[0]))) {
                monitor.checkCancelled();
                String[] fields = row.split("\t", 2);
                if (fields.length != 2) continue;
                Address address = toAddr(Long.decode(fields[0]));
                Function f = getFunctionAt(address);
                if (f == null) {
                    index.append(fields[0] + "\t" + fields[1] + "\tfalse\tmissing_function\n");
                    continue;
                }
                DecompileResults result = decompiler.decompileFunction(f, 30, monitor);
                String name = address.toString() + ".ghidra.txt";
                StringBuilder text = new StringBuilder("UNREVIEWED GHIDRA DRAFT: types, signedness, aliasing and calling convention require original-assembly review.\nCompiler register-save/restore helpers are annotated as register-preserving bookkeeping for this export only.\n");
                text.append("Symbol: " + fields[1] + "\nAddress: " + address + "\n\n");
                if (result.decompileCompleted()) text.append(result.getDecompiledFunction().getC());
                else text.append("Decompilation failed: " + result.getErrorMessage());
                text.append("\n\nIncoming references (up to 64):\n");
                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(address);
                int count = 0;
                while (refs.hasNext() && count++ < 64) {
                    Reference r = refs.next();
                    Function caller = getFunctionContaining(r.getFromAddress());
                    text.append(r.getFromAddress() + " " + r.getReferenceType() + " " + (caller == null ? "" : caller.getName()) + "\n");
                }
                text.append("\nCalled functions:\n");
                for (Function called : f.getCalledFunctions(monitor))
                    text.append(called.getEntryPoint() + " " + called.getName() + "\n");
                Files.writeString(output.resolve(name), text.toString(), StandardCharsets.UTF_8);
                index.append(fields[0] + "\t" + fields[1] + "\t" + result.decompileCompleted() + "\t" + name + "\n");
            }
        } finally { decompiler.dispose(); }
        Files.writeString(output.resolve("index.tsv"), index.toString(), StandardCharsets.UTF_8);
        println("B8 Ghidra context exported to " + output);
    }
}
