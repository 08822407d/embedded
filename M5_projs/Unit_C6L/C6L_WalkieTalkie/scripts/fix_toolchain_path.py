Import("env")

import os


def find_toolchain_bin(package_dir, compiler_name):
    candidates = [os.path.join(package_dir, "bin")]

    try:
        for entry in os.listdir(package_dir):
            nested_bin = os.path.join(package_dir, entry, "bin")
            candidates.append(nested_bin)
    except OSError:
        return None

    for candidate in candidates:
        compiler_path = os.path.join(candidate, compiler_name)
        if os.path.exists(compiler_path):
            return candidate

    return None


toolchain_dir = env.PioPlatform().get_package_dir("toolchain-riscv32-esp")
compiler_name = "riscv32-esp-elf-g++.exe" if os.name == "nt" else "riscv32-esp-elf-g++"

if toolchain_dir:
    bin_dir = find_toolchain_bin(toolchain_dir, compiler_name)
    if bin_dir:
        env.PrependENVPath("PATH", bin_dir)
