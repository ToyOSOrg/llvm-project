// UNSUPPORTED: system-windows

// RUN: %clang -### %s --target=x86_64-unknown-toyos \
// RUN:     -resource-dir=%S/Inputs/resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefixes=CHECK,CHECK-X86_64 %s
// RUN: %clang -### %s --target=aarch64-unknown-toyos \
// RUN:     -resource-dir=%S/Inputs/resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefixes=CHECK,CHECK-AARCH64 %s
// CHECK: "-cc1"
// CHECK-X86_64-SAME: "-triple" "x86_64-unknown-toyos"
// CHECK-AARCH64-SAME: "-triple" "aarch64-unknown-toyos"
// CHECK-SAME: "-pic-level" "2" "-pic-is-pie"
// CHECK-SAME: "-funwind-tables=2"
// CHECK-SAME: "-resource-dir" "[[RESOURCE_DIR:[^"]+]]"
// CHECK-SAME: "-isysroot" "[[SYSROOT:[^"]+]]"
// CHECK-SAME: "-internal-isystem" "[[RESOURCE_DIR]]{{/|\\\\}}include"
// CHECK-SAME: "-internal-externc-isystem" "[[SYSROOT]]{{/|\\\\}}include"
// CHECK: {{.*}}ld.lld{{.*}}" "--sysroot=[[SYSROOT]]"
// CHECK-SAME: "-pie"
// CHECK-SAME: "--eh-frame-hdr"
// CHECK-NOT: "-dynamic-linker"
// CHECK-SAME: "-o" "a.out"
// CHECK-NOT: crt0.o
// CHECK-SAME: "-L[[SYSROOT]]{{/|\\\\}}lib"
// CHECK-SAME: "{{.*}}.o"
// CHECK-SAME: "-ltoyos_c"
// CHECK-NOT: clang_rt

// RUN: %clang -### %s --target=x86_64-unknown-toyos -shared \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-SHARED %s
// CHECK-SHARED: {{.*}}ld.lld{{.*}}" "--sysroot={{[^"]+}}"
// CHECK-SHARED-NOT: "-pie"
// CHECK-SHARED-SAME: "-shared" "-Bsymbolic"
// CHECK-SHARED-SAME: "-ltoyos_c"

// RUN: %clang -### %s --target=x86_64-unknown-toyos -nostdlib \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-NOSTDLIB %s
// CHECK-NOSTDLIB: {{.*}}ld.lld{{.*}}" "--sysroot={{[^"]+}}" "-pie"
// CHECK-NOSTDLIB-NOT: "-ltoyos_c"

// RUN: %clang -### %s --target=x86_64-unknown-toyos -nolibc \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-NOLIBC %s
// CHECK-NOLIBC: {{.*}}ld.lld{{.*}}" "--sysroot={{[^"]+}}" "-pie"
// CHECK-NOLIBC-NOT: "-ltoyos_c"

// RUN: %clang -### %s --target=x86_64-unknown-toyos -r \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-RELOCATABLE %s
// CHECK-RELOCATABLE: {{.*}}ld.lld{{.*}}" "--sysroot={{[^"]+}}" "-r"
// CHECK-RELOCATABLE-NOT: "-pie"
// CHECK-RELOCATABLE-NOT: crt0.o
// CHECK-RELOCATABLE-NOT: "-ltoyos_c"

// RUN: %clang -### %s --target=x86_64-unknown-toyos -nostdlibinc \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-NOSTDLIBINC %s
// CHECK-NOSTDLIBINC: "-cc1"
// CHECK-NOSTDLIBINC-NOT: "-internal-externc-isystem"

// RUN: %clang -dM -E %s --target=x86_64-unknown-toyos 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-MACROS %s
// RUN: %clang -dM -E %s --target=aarch64-unknown-toyos 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-MACROS %s
// CHECK-MACROS-DAG: #define __ELF__ 1
// CHECK-MACROS-DAG: #define __toyos__ 1

// RUN: %clang -dM -E %s --target=x86_64-unknown-toyos 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-NOT-UNIX %s
// CHECK-NOT-UNIX-NOT: #define __unix__
