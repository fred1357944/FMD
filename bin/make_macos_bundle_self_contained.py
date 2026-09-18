#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

SHALLOW_QML_KEEP = {
    "QML",
    "Qt",
    "QtQml",
    "QtQuick",
    "QtQuick/Controls",
}

RECURSIVE_QML_KEEP = {
    "Qt/labs/folderlistmodel",
    "QtQml/Models",
    "QtQml/WorkerScript",
    "QtQuick/Controls/Basic",
    "QtQuick/Controls/Material",
    "QtQuick/Controls/impl",
    "QtQuick/Dialogs",
    "QtQuick/Layouts",
    "QtQuick/Templates",
    "QtQuick/Window",
}

KEEP_QUICK_PLUGINS = {
    "libmodelsplugin.dylib",
    "libqmlfolderlistmodelplugin.dylib",
    "libqmlplugin.dylib",
    "libqquicklayoutsplugin.dylib",
    "libqtquick2plugin.dylib",
    "libqtquickcontrols2basicstyleimplplugin.dylib",
    "libqtquickcontrols2basicstyleplugin.dylib",
    "libqtquickcontrols2implplugin.dylib",
    "libqtquickcontrols2materialstyleimplplugin.dylib",
    "libqtquickcontrols2materialstyleplugin.dylib",
    "libqtquickcontrols2plugin.dylib",
    "libqtquickdialogs2quickimplplugin.dylib",
    "libqtquickdialogsplugin.dylib",
    "libqtquicktemplates2plugin.dylib",
    "libquickwindowplugin.dylib",
    "libworkerscriptplugin.dylib",
}


def run(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, check=check, text=True, capture_output=True)


def is_macho(path: Path) -> bool:
    if path.is_symlink() or not path.is_file():
        return False
    try:
        out = run("file", "-b", str(path)).stdout
    except subprocess.CalledProcessError:
        return False
    return "Mach-O" in out


def machos(root: Path):
    for path in root.rglob("*"):
        if is_macho(path):
            yield path


def dylib_lines(path: Path) -> list[str]:
    return [line.strip().split(" ")[0] for line in run("otool", "-L", str(path)).stdout.splitlines()[1:] if line.strip()]


def rpaths(path: Path) -> list[str]:
    lines = run("otool", "-l", str(path)).stdout.splitlines()
    out: list[str] = []
    for i, line in enumerate(lines):
        if line.strip() != "cmd LC_RPATH":
            continue
        for j in range(i + 1, min(i + 8, len(lines))):
            if "path " in lines[j]:
                out.append(lines[j].split("path ", 1)[1].split(" (offset", 1)[0].strip())
                break
    return out


def shallow_copy_dir(src: Path, dst: Path) -> None:
    dst.mkdir(parents=True, exist_ok=True)
    for entry in src.iterdir():
        target = dst / entry.name
        if entry.is_symlink():
            if target.exists() or target.is_symlink():
                target.unlink()
            os.symlink(os.readlink(entry), target)
        elif entry.is_file():
            shutil.copy2(entry, target, follow_symlinks=False)


def prune_qml_tree(app: Path) -> None:
    qml_root = app / "Contents/Resources/qml"
    stage = app / "Contents/Resources/qml.minimal"
    backup = app / "Contents/Resources/qml.full"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    for rel in sorted(SHALLOW_QML_KEEP):
        shallow_copy_dir(qml_root / rel, stage / rel)
    for rel in sorted(RECURSIVE_QML_KEEP):
        shutil.copytree(qml_root / rel, stage / rel, symlinks=True, dirs_exist_ok=True)
    if backup.exists():
        shutil.rmtree(backup)
    qml_root.rename(backup)
    stage.rename(qml_root)


def prune_quick_plugins(app: Path) -> None:
    quick = app / "Contents/PlugIns/quick"
    for child in list(quick.iterdir()):
        if child.name not in KEEP_QUICK_PLUGINS:
            child.unlink()


def build_framework_index(frameworks_root: Path) -> dict[str, Path]:
    index: dict[str, Path] = {}
    for path in machos(frameworks_root):
        rel = path.relative_to(frameworks_root).as_posix()
        index[rel] = path
        index[path.name] = path
        if ".framework/" in rel:
            prefix, _ = rel.split(".framework/", 1)
            index[f"{prefix}.framework/{path.name}"] = path
            index[f"{prefix}.framework"] = path
    return index


def resolve_bundle_dep(dep: str, index: dict[str, Path]) -> Path | None:
    if "../Frameworks/" in dep:
        rel = dep.split("../Frameworks/", 1)[1]
        return index.get(rel) or index.get(Path(rel).name)
    if dep.startswith("@rpath/"):
        rel = dep[len("@rpath/") :]
        return index.get(rel) or index.get(Path(rel).name)
    return index.get(Path(dep).name)


def prune_frameworks(app: Path) -> set[str]:
    frameworks_root = app / "Contents/Frameworks"
    index = build_framework_index(frameworks_root)
    roots = [p for p in machos(app / "Contents") if frameworks_root not in p.parents]
    keep: set[Path] = set()
    stack: list[Path] = []
    for root in roots:
        for dep in dylib_lines(root):
            target = resolve_bundle_dep(dep, index)
            if target and target not in keep:
                keep.add(target)
                stack.append(target)
    while stack:
        current = stack.pop()
        for dep in dylib_lines(current):
            target = resolve_bundle_dep(dep, index)
            if target and target not in keep:
                keep.add(target)
                stack.append(target)
    kept_children = {target.relative_to(frameworks_root).parts[0] for target in keep}
    for child in list(frameworks_root.iterdir()):
        if child.name in kept_children:
            continue
        if child.is_dir() and not child.is_symlink():
            shutil.rmtree(child)
        else:
            child.unlink()
    return kept_children


def rewrite_homebrew_refs(app: Path) -> None:
    frameworks_root = app / "Contents/Frameworks"
    index = build_framework_index(frameworks_root)
    for path in machos(app / "Contents"):
        rel_app = path.relative_to(app).as_posix()
        if rel_app.startswith("Contents/Frameworks/"):
            rel_fw = path.relative_to(frameworks_root).as_posix()
            new_id = f"@executable_path/../Frameworks/{rel_fw}"
            ids = run("otool", "-D", str(path), check=False).stdout.splitlines()[1:]
            for current_id in ids:
                if current_id.startswith("/opt/homebrew"):
                    subprocess.run(["install_name_tool", "-id", new_id, str(path)], check=True)
                    break
        for dep in dylib_lines(path):
            if not dep.startswith("/opt/homebrew"):
                continue
            target = index.get(Path(dep).name)
            if not target:
                continue
            rel_fw = target.relative_to(frameworks_root).as_posix()
            new_dep = f"@executable_path/../Frameworks/{rel_fw}"
            subprocess.run(["install_name_tool", "-change", dep, new_dep, str(path)], check=True)
        for rpath in rpaths(path):
            if rpath.startswith("/opt/homebrew"):
                subprocess.run(["install_name_tool", "-delete_rpath", rpath, str(path)], check=True)


def audit_homebrew_refs(app: Path) -> tuple[list[str], list[str]]:
    bad_deps: list[str] = []
    bad_rpaths: list[str] = []
    for path in machos(app / "Contents"):
        deps = [dep for dep in dylib_lines(path) if "/opt/homebrew" in dep]
        if deps:
            bad_deps.append(f"{path.relative_to(app)} -> {deps}")
        rps = [rpath for rpath in rpaths(path) if "/opt/homebrew" in rpath]
        if rps:
            bad_rpaths.append(f"{path.relative_to(app)} -> {rps}")
    return bad_deps, bad_rpaths


def smoke_launch(app: Path, seconds: int) -> str:
    exe = app / "Contents/MacOS/fmd"
    proc = subprocess.Popen([str(exe)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    try:
        time.sleep(seconds)
        if proc.poll() is None:
            proc.terminate()
            out, _ = proc.communicate(timeout=10)
            return f"alive for {seconds}s\n{out or ''}"
        out, _ = proc.communicate(timeout=10)
        return f"exited rc={proc.returncode}\n{out or ''}"
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait(timeout=10)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_app", type=Path)
    parser.add_argument("output_app", type=Path)
    parser.add_argument("--smoke-seconds", type=int, default=5)
    args = parser.parse_args()

    src = args.input_app.resolve()
    dst = args.output_app.resolve()
    if dst.exists():
        raise SystemExit(f"Refusing to overwrite existing output: {dst}")
    shutil.copytree(src, dst, symlinks=True)

    prune_qml_tree(dst)
    prune_quick_plugins(dst)
    kept_frameworks = prune_frameworks(dst)
    rewrite_homebrew_refs(dst)
    subprocess.run(["codesign", "--force", "--sign", "-", "--timestamp=none", "--deep", str(dst)], check=False)

    bad_deps, bad_rpaths = audit_homebrew_refs(dst)
    print("output_app:", dst)
    print("qml_shallow_keep:", sorted(SHALLOW_QML_KEEP))
    print("qml_recursive_keep:", sorted(RECURSIVE_QML_KEEP))
    print("quick_plugins_keep:", sorted(KEEP_QUICK_PLUGINS))
    print("framework_children_keep:", len(kept_frameworks))
    print("bad_dep_count:", len(bad_deps))
    for row in bad_deps:
        print("  ", row)
    print("bad_rpath_count:", len(bad_rpaths))
    for row in bad_rpaths:
        print("  ", row)
    print("launch:", smoke_launch(dst, args.smoke_seconds))
    return 0 if not bad_deps and not bad_rpaths else 1


if __name__ == "__main__":
    sys.exit(main())
