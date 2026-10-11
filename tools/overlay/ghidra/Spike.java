// Headless spike: decompile selected Turbo Pascal overlay functions with overlay metadata applied.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.pcode.*;
import java.io.*;
import java.math.BigInteger;
import java.nio.file.*;
import java.util.*;

public class Spike extends GhidraScript {
    String dir = System.getenv("SPIKE_DIR");
    int UNIT_SEG = 0x2000, DATA_SEG = 0x9000;

    Address seg(int s, int off) throws Exception {
        return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(((long)s << 4) + off);
    }
    public void run() throws Exception {
        String[] args = getScriptArgs(); boolean bare = args.length>0 && args[0].equals("bare"); String sfx = bare ? "_bare" : "";
        println("MODE: " + (bare ? "bare (no DS context, no stub labels)" : "metadata applied"));
        Memory mem = currentProgram.getMemory();
        // 1) unit 0x1c code block at 2000:0000, data block at 9000:0000 (uninitialized globals)
        byte[] code = Files.readAllBytes(Paths.get(dir, "unit_code.bin"));
        MemoryBlock ub = mem.createInitializedBlock("unit", seg(UNIT_SEG,0), new ByteArrayInputStream(code), code.length, monitor, false);
        ub.setExecute(true); ub.setRead(true);
        MemoryBlock db = mem.createUninitializedBlock("ovl_data", seg(DATA_SEG,0), 0x10000, false);
        db.setRead(true); db.setWrite(true);
        // 2) DS context so [imm16] globals resolve into ovl_data
        if (!bare) { Register ds = currentProgram.getRegister("DS");
        currentProgram.getProgramContext().setValue(ds, ub.getStart(), ub.getEnd(), BigInteger.valueOf(DATA_SEG)); }
        // 3) stub labels in the resident image (loaded at 0000:0000 by the importer)
        int nstub=0;
        if (!bare) for (String line : Files.readAllLines(Paths.get(dir, "stubs.txt"))) {
            String[] p = line.trim().split(" ");
            long lin = Long.decode(p[0]);
            Address a = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(lin);
            String name = "ovl_u" + p[1].substring(2) + "_e" + p[2] + "__ovr_" + Integer.toHexString(Integer.parseInt(p[3]));
            createLabel(a, name, true);   // label only: the resident block is the packed file image
            nstub++;
        }
        println("stub labels: " + nstub);
        // 4) targets (unit-relative offsets)
        // targets: SPIKE_TARGETS="rel:name,rel:name" (unit-relative hex offsets); default = ECL interpreter quartet in unit 0x1c
        String tspec = System.getenv("SPIKE_TARGETS");
        if (tspec == null || tspec.isEmpty()) tspec = "3171:ecl_run,2f2d:ecl_dispatch,0001:ecl_op00_stop,0bd1:ecl_op20_newecl";
        String[] parts = tspec.split(",");
        int[] rel = new int[parts.length]; String[] nm = new String[parts.length];
        for (int i = 0; i < parts.length; i++) { String[] kv = parts[i].split(":"); rel[i] = Integer.parseInt(kv[0], 16); nm[i] = kv[1]; }
        // fixups that are not far-call segment words are far-pointer segments to this unit's own
        // constants: patch them to the unit's assigned segment so the decompiler sees real pointers
        if (!bare) { int patched = 0;
            for (String line : Files.readAllLines(Paths.get(dir, "fixups.txt"))) {
                int fx = Integer.parseInt(line.trim());
                if (fx >= 3 && (code[fx - 3] & 0xff) == 0x9A) continue;
                Address w = seg(UNIT_SEG, fx);
                mem.setShort(w, (short) UNIT_SEG); patched++;
            }
            println("non-call fixups patched to unit segment: " + patched); }
        for (int i=0;i<rel.length;i++) {
            Address a = seg(UNIT_SEG, rel[i]);
            disassemble(a);
            Function f = createFunction(a, nm[i]);
            if (f==null) f = getFunctionAt(a);
            try { f.setCallingConvention("__stdcall16far"); } catch (Exception e) { println("cc not set: "+e.getMessage()); }
        }
        analyzeAll(currentProgram);
        // 5) decompile and measure
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        for (int i=0;i<rel.length;i++) {
            Function f = getFunctionAt(seg(UNIT_SEG, rel[i]));
            if (f==null) { println("NO FUNCTION "+nm[i]); continue; }
            DecompileResults r = di.decompileFunction(f, 60, monitor);
            String c = r.getDecompiledFunction()==null ? "<decompile failed: "+r.getErrorMessage()+">" : r.getDecompiledFunction().getC();
            int calls=0, named=0, stubCalls=0, unresolved=0;
            HighFunction hf = r.getHighFunction();
            if (hf!=null) {
                Iterator<PcodeOpAST> it = hf.getPcodeOps();
                while (it.hasNext()) { PcodeOpAST op = it.next();
                    if (op.getOpcode()==PcodeOp.CALL) { calls++;
                        Address t = op.getInput(0).getAddress(); Function tf = getFunctionAt(t); Symbol s = getSymbolAt(t);
                        if (s!=null && s.getName().startsWith("ovl_u")) stubCalls++;
                        else if (tf!=null && !tf.getName().startsWith("FUN_")) named++;
                    } else if (op.getOpcode()==PcodeOp.CALLIND) { calls++; unresolved++; }
                }
            }
            int gotos=count(c,"goto "), halts=count(c,"halt_baddata"), unaff=count(c,"unaff_")+count(c,"in_")+count(c,"extraout_"), lines=c.split("\n").length;
            println("==== "+nm[i]+" @ "+f.getEntryPoint()+" size="+f.getBody().getNumAddresses()+" params="+f.getParameterCount()+" frame="+f.getStackFrame().getFrameSize()
                +" calls="+calls+" to_overlay_stub="+stubCalls+" to_named="+named+" indirect="+unresolved+" gotos="+gotos+" baddata="+halts+" unaff/in/extraout="+unaff+" c_lines="+lines);
            Files.writeString(Paths.get(dir, nm[i]+sfx+".c"), c);
        }
        di.dispose();
    }
    int count(String s, String k){ int n=0,i=0; while((i=s.indexOf(k,i))>=0){n++;i+=k.length();} return n; }
}
