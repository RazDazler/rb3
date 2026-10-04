// Export selected local Ghidra drafts and call references. Never edit source.
// @author ChatGPT
// @category RockBand3
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.listing.Variable;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.lang.InjectPayload;
import ghidra.program.model.lang.Register;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.data.VoidDataType;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.FloatDataType;
import ghidra.program.model.data.IntegerDataType;
import ghidra.program.model.data.BooleanDataType;
import ghidra.program.model.data.PointerDataType;
import ghidra.xml.XmlPullParserFactory;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.charset.StandardCharsets;
import java.math.BigInteger;

public class ExportB8Context extends GhidraScript {
    private DataType type(char code) {
        switch (code) {
            case 'f': return FloatDataType.dataType;
            case 'i': return IntegerDataType.dataType;
            case 'b': return BooleanDataType.dataType;
            case 'F': return new PointerDataType(FloatDataType.dataType, currentProgram.getDataTypeManager());
            case 'I': return new PointerDataType(IntegerDataType.dataType, currentProgram.getDataTypeManager());
            case 'p': return new PointerDataType(VoidDataType.dataType, currentProgram.getDataTypeManager());
            default: return VoidDataType.dataType;
        }
    }

    private void signature(Function f, char returns, String codes, String names) throws Exception {
        String[] labels = names.split(",");
        Variable[] parameters = new Variable[codes.length()];
        for (int i = 0; i < parameters.length; i++)
            parameters[i] = new ParameterImpl(labels[i], type(codes.charAt(i)), currentProgram);
        f.updateFunction(currentProgram.getCompilerSpec().getDefaultCallingConvention().getName(),
            new ReturnParameterImpl(type(returns), currentProgram),
            Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true,
            SourceType.USER_DEFINED, parameters);
        println("Reviewed vocal signature: " + f.getName() + " " + f.getSignature());
        int floatRegister = 1, generalRegister = 3;
        for (ghidra.program.model.listing.Parameter p : f.getParameters()) {
            String expected = p.getDataType() instanceof FloatDataType
                ? "f" + floatRegister++ : "r" + generalRegister++;
            if (!p.getVariableStorage().toString().startsWith(expected + ":"))
                throw new IllegalStateException("Unexpected Wii parameter storage: " +
                    f.getName() + " " + p.getName() + " " + p.getVariableStorage());
            println("  " + p.getName() + " " + p.getVariableStorage());
        }
    }

    private void vocalSignature(Function f) throws Exception {
        // Exact names only. Pointer pointees remain opaque except primitive
        // references; signatures come from headers and reviewed B8 call/return
        // assembly, not guesses from the decompiler's generated C.
        switch (f.getName()) {
            case "GetSloppyPitch__9VocalPartCFfifRf":
                signature(f, 'f', "pfifF", "self,ms,noteIndex,pitch,targetMs"); break;
            case "GetNoteSliceWeight__9VocalPartCFffi":
                signature(f, 'f', "pffi", "self,startMs,endMs,noteIndex"); break;
            case "CalculateScore__9VocalPartCFfifR15VocalScoreCache":
                signature(f, 'v', "pfifp", "self,ms,noteIndex,hit,cache"); break;
            case "CouldScoreAgainstPart__9VocalPartFfP12TalkyMatcherffRf":
                signature(f, 'b', "pfpffF", "self,ms,talkyMatcher,pitch,margin,targetPitch"); break;
            case "ScoreNote__9VocalPartCFfiRfRiRfRf":
                signature(f, 'f', "pfiFIFF", "self,ms,noteIndex,pitch,octaveOffset,targetPitch,targetMs"); break;
            case "GetNoteRange__9VocalPartFfRiRi":
                signature(f, 'v', "pfII", "self,ms,first,last"); break;
            case "PitchBetween__FfffRf":
                signature(f, 'b', "fffF", "pitch,a,b,adjusted"); break;
        }
    }

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
            vocalSignature(helper);
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
                // B8 __OSPSInit clears GQR0; the only other named writer is
                // MetroTRK restoring its saved value. Ordinary game routines
                // use qr0 for unquantized float register saves, not ldexpf.
                // Limit this analysis assumption to requested game functions.
                Register gqr0 = currentProgram.getRegister("GQR0");
                boolean ordinaryVocalRoutine = gqr0 != null && fields[1].contains("__9VocalPart");
                if (ordinaryVocalRoutine) {
                    currentProgram.getProgramContext().setValue(gqr0,
                        f.getBody().getMinAddress(), f.getBody().getMaxAddress(), BigInteger.ZERO);
                    decompiler.flushCache();
                }
                DecompileResults result = decompiler.decompileFunction(f, 30, monitor);
                String name = address.toString() + ".ghidra.txt";
                StringBuilder text = new StringBuilder("UNREVIEWED GHIDRA DRAFT: types, signedness, aliasing and calling convention require original-assembly review.\nCompiler register-save/restore helpers are annotated as register-preserving bookkeeping for this export only.\n");
                text.append("Selected vocal signatures are annotated from headers and reviewed B8 assembly; opaque pointees and unlisted signatures remain inferred.\n");
                if (ordinaryVocalRoutine)
                    text.append("Analysis assumption: GQR0 = 0 in this vocal routine, based on B8 __OSPSInit and the named writer audit.\n");
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
