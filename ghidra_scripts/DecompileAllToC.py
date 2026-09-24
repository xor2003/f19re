## ###
# IP: GHIDRA
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This headless script decompiles all functions and writes each result as a .c file.
# Usage:
#   -postScript DecompileAllToC.py /path/to/output_dir
#
# @category: Decompile
# @runtime Jython

import os
import re

from ghidra.app.decompiler import DecompInterface
from ghidra.app.decompiler import DecompileOptions
from ghidra.util.task import ConsoleTaskMonitor


def sanitize_name(name):
    return re.sub(r"[^A-Za-z0-9_\\-\\.]", "_", name)


def run():
    if len(getScriptArgs()) > 0:
        out_dir = getScriptArgs()[0]
    else:
        out_dir = os.path.join(os.getcwd(), "decompiled")

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    decompiler = DecompInterface()
    opts = DecompileOptions()
    decompiler.setOptions(opts)
    decompiler.setSimplificationStyle("decompile")
    if not decompiler.openProgram(currentProgram):
        print("Unable to initialize DecompInterface for current program"); print(decompiler.getLastMessage())
        return

    fm = currentProgram.getFunctionManager()
    monitor = ConsoleTaskMonitor()

    total = 0
    ok = 0
    it = fm.getFunctions(True)
    while it.hasNext():
        total += 1
        func = it.next()
        try:
            res = decompiler.decompileFunction(func, 120, monitor)
            if not res.decompileCompleted():
                print("FAILED: %s @ %s -> %s" % (func.getName(), func.getEntryPoint(), res.getErrorMessage()))
                continue

            dec = res.getDecompiledFunction()
            csrc = dec.getC()

            fname = sanitize_name(func.getName() or "sub_" + str(func.getEntryPoint()))
            faddr = str(func.getEntryPoint()).replace(":", "_")
            outfile = os.path.join(out_dir, "%s_%s.c" % (fname, faddr))
            f = open(outfile, "w")
            try:
                f.write(csrc)
            finally:
                f.close()
            ok += 1
        except Exception as e:
            print("EXCEPTION: %s @ %s -> %s" % (func.getName(), func.getEntryPoint(), e))

    decompiler.closeProgram()
    decompiler.dispose()

    print("DECOMPILE_DONE total=%d ok=%d out_dir=%s" % (total, ok, out_dir))


run()
