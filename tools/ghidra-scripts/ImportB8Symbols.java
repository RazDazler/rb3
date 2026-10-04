// Import the repository's B8 addresses and function boundaries before analysis.
// @author ChatGPT
// @category RockBand3
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.ArrayList;

public class ImportB8Symbols extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("symbols.txt required");
        Pattern row = Pattern.compile("^(\\S+) = \\S+:(0x[0-9A-Fa-f]+);.*$");
        Pattern size = Pattern.compile("size:(0x[0-9A-Fa-f]+|[0-9]+)");
        int labels = 0, functions = 0, errors = 0;
        ArrayList<Function> imported = new ArrayList<>();
        for (String line : Files.readAllLines(Paths.get(args[0]))) {
            monitor.checkCancelled();
            Matcher m = row.matcher(line);
            if (!m.matches()) continue;
            Address address = toAddr(Long.decode(m.group(2)));
            if (!currentProgram.getMemory().contains(address)) continue;
            try {
                createLabel(address, m.group(1), true, SourceType.IMPORTED);
                labels++;
                Matcher s = size.matcher(line);
                if (line.contains("type:function") && s.find()) {
                    long length = Long.decode(s.group(1));
                    if (length <= 0 || !currentProgram.getMemory().contains(address.add(length - 1))) continue;
                    Function f = getFunctionAt(address);
                    if (f == null) f = currentProgram.getFunctionManager().createFunction(
                        m.group(1), address, new AddressSet(address, address.add(length - 1)), SourceType.IMPORTED);
                    if (f != null) { functions++; imported.add(f); }
                }
            } catch (Exception e) {
                if (++errors <= 10) println("Import warning: " + m.group(1) + ": " + e.getMessage());
            }
        }
        for (Function f : imported) {
            monitor.checkCancelled();
            new DisassembleCommand(f.getEntryPoint(), f.getBody(), true).applyTo(currentProgram, monitor);
        }
        println("B8 symbols: " + labels + " labels, " + functions + " functions, " + errors + " warnings");
    }
}
