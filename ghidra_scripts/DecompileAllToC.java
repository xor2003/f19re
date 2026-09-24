/* ###
 * IP: GHIDRA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.util.regex.Pattern;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

//@category=Decompiler
public class DecompileAllToC extends GhidraScript {

	private static final Pattern SAFE_NAME = Pattern.compile("[^A-Za-z0-9_\\-\\.]");

	@Override
	public void run() throws Exception {
		String outDirPath = "/home/xor/vextest/SORTD_decomp";
		String[] args = getScriptArgs();
		if (args != null && args.length > 0 && args[0] != null && args[0].trim().length() > 0) {
			outDirPath = args[0];
		}

		File outDir = new File(outDirPath);
		if (!outDir.exists()) {
			outDir.mkdirs();
		}

		DecompInterface decompiler = new DecompInterface();
		DecompileOptions options = new DecompileOptions();
		decompiler.setOptions(options);
		decompiler.setSimplificationStyle("decompile");

		if (!decompiler.openProgram(currentProgram)) {
			printerr("Unable to initialize DecompInterface");
			return;
		}

		FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
		int total = 0;
		int ok = 0;

		while (it.hasNext()) {
			Function f = it.next();
			total++;
			String name = f.getName();
			long timeout = 120;
			DecompileResults results = decompiler.decompileFunction(f, timeout, monitor);
			if (!results.decompileCompleted()) {
				printerr("FAILED: " + name + " @ " + f.getEntryPoint() + " -> " + results.getErrorMessage());
				continue;
			}

			String src = results.getDecompiledFunction().getC();
			String safeName = SAFE_NAME.matcher(name).replaceAll("_");
			String addr = f.getEntryPoint().toString().replace(":", "_");
			File outFile = new File(outDir, safeName + "_" + addr + ".c");
			BufferedWriter writer = new BufferedWriter(new FileWriter(outFile));
			try {
				writer.write(src);
			}
			finally {
				writer.close();
			}
			ok++;
		}

		decompiler.closeProgram();
		decompiler.dispose();

		println("DECOMPILE_DONE total=" + total + " ok=" + ok + " out_dir=" + outDir.getAbsolutePath());
	}
}
