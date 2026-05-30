import argparse
import glob
import os
import re
import subprocess
import sys


def run(args, **kwargs):
    return subprocess.check_output(args, **kwargs)


def preprocess_source(cc, include_dirs, source):
    cmd = [cc, "-E", "-std=c99", "-DNO_QSTR"]
    for include_dir in include_dirs:
        cmd.append("-I" + include_dir)
    cmd.append(source)
    return run(cmd, text=True, errors="replace")


def collect_from_sources(cc, include_dirs, sources):
    qstrs = set()
    modules = []
    roots = []

    qstr_re = re.compile(r"MP_QSTR_[_a-zA-Z0-9]+")
    module_re = re.compile(
        r"(?:MP_REGISTER_MODULE|MP_REGISTER_EXTENSIBLE_MODULE|MP_REGISTER_MODULE_DELEGATION)\(.*?,\s*.*?\);"
    )
    root_re = re.compile(r"MP_REGISTER_ROOT_POINTER\(.*?\);")

    for source in sources:
        output = preprocess_source(cc, include_dirs, source)
        for line in output.splitlines():
            for match in qstr_re.findall(line):
                qstrs.add(match.replace("MP_QSTR_", ""))
            modules.extend(module_re.findall(line))
            roots.extend(root_re.findall(line))

    return qstrs, modules, roots


def preprocess_qstr_defs(cc, include_dirs, qstrdefs_path, qstrs, out_path):
    with open(qstrdefs_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    lines.extend(f"Q({name})\n" for name in sorted(qstrs))
    protected = "".join(
        f'"{line.rstrip()}"\n' if line.startswith("Q(") else line
        for line in lines
    )

    cmd = [cc, "-E", "-std=c99", "-"]
    for include_dir in include_dirs:
        cmd.append("-I" + include_dir)

    output = run(cmd, input=protected, text=True, errors="replace")
    unwrapped = []
    unwrap_re = re.compile(r'^"(Q\(.*\))"$')
    for line in output.splitlines():
        match = unwrap_re.match(line)
        if match:
            unwrapped.append(match.group(1))
        elif line.startswith("QCFG("):
            unwrapped.append(line)

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(unwrapped))
        f.write("\n")


def write_lines(path, lines):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        for line in lines:
            f.write(line)
            f.write("\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", required=True)
    parser.add_argument("--python", required=True)
    parser.add_argument("--mp-top", required=True)
    parser.add_argument("--embed-dir", required=True)
    parser.add_argument("--port-dir", required=True)
    args = parser.parse_args()

    genhdr = os.path.join(args.embed_dir, "genhdr")
    os.makedirs(genhdr, exist_ok=True)

    include_dirs = [
        args.port_dir,
        args.embed_dir,
        os.path.join(args.embed_dir, "port"),
        os.path.join(args.embed_dir, "genhdr"),
        os.path.join(args.embed_dir, "py"),
        args.mp_top,
        os.path.join(args.mp_top, "ports", "embed"),
    ]

    sources = []
    sources.extend(glob.glob(os.path.join(args.embed_dir, "py", "*.c")))
    sources.extend(glob.glob(os.path.join(args.embed_dir, "shared", "runtime", "*.c")))
    sources.extend(glob.glob(os.path.join(args.embed_dir, "port", "*.c")))
    sources.extend(glob.glob(os.path.join(args.mp_top, "extmod", "modjson.c")))
    sources.extend(glob.glob(os.path.join(args.mp_top, "extmod", "modrandom.c")))
    sources.extend(glob.glob(os.path.join(args.mp_top, "extmod", "modheapq.c")))
    sources.extend(glob.glob(os.path.join(args.mp_top, "extmod", "modbinascii.c")))
    sources.extend(glob.glob(os.path.join(args.mp_top, "extmod", "modhashlib.c")))

    qstrs, modules, roots = collect_from_sources(args.cc, include_dirs, sorted(sources))

    qstr_preprocessed = os.path.join(genhdr, "qstrdefs.preprocessed.h")
    preprocess_qstr_defs(
        args.cc,
        include_dirs,
        os.path.join(args.mp_top, "py", "qstrdefs.h"),
        qstrs,
        qstr_preprocessed,
    )

    with open(os.path.join(genhdr, "qstrdefs.generated.h"), "w", encoding="utf-8", newline="\n") as out:
        subprocess.check_call(
            [args.python, os.path.join(args.mp_top, "py", "makeqstrdata.py"), qstr_preprocessed],
            stdout=out,
        )

    moduledefs_collected = os.path.join(genhdr, "moduledefs.collected")
    write_lines(moduledefs_collected, sorted(set(modules)))
    with open(os.path.join(genhdr, "moduledefs.h"), "w", encoding="utf-8", newline="\n") as out:
        subprocess.check_call(
            [args.python, os.path.join(args.mp_top, "py", "makemoduledefs.py"), moduledefs_collected],
            stdout=out,
        )

    roots_collected = os.path.join(genhdr, "root_pointers.collected")
    write_lines(roots_collected, sorted(set(roots)))
    with open(os.path.join(genhdr, "root_pointers.h"), "w", encoding="utf-8", newline="\n") as out:
        subprocess.check_call(
            [args.python, os.path.join(args.mp_top, "py", "make_root_pointers.py"), roots_collected],
            stdout=out,
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
