// UNSUPPORTED: system-windows

// RUN: %clangxx -### %s --target=x86_64-unknown-toyos \
// RUN:     -resource-dir=%S/Inputs/resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck %s
// RUN: %clangxx -### %s --target=aarch64-unknown-toyos \
// RUN:     -resource-dir=%S/Inputs/resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck %s
// CHECK: "-cc1"
// CHECK-SAME: "-resource-dir" "[[RESOURCE_DIR:[^"]+]]"
// CHECK-SAME: "-isysroot" "[[SYSROOT:[^"]+]]"
// CHECK-SAME: "-internal-isystem" "[[SYSROOT]]{{/|\\\\}}include{{/|\\\\}}c++{{/|\\\\}}v1"
// CHECK-SAME: "-internal-isystem" "[[RESOURCE_DIR]]{{/|\\\\}}include"
// CHECK-SAME: "-internal-externc-isystem" "[[SYSROOT]]{{/|\\\\}}include"
// CHECK: {{.*}}ld.lld{{.*}}" "--sysroot=[[SYSROOT]]"
// CHECK-SAME: "{{.*}}.o" "-lc++" "-ltoyos_c"

// RUN: %clangxx -### %s --target=x86_64-unknown-toyos -nostdinc++ \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-NOSTDINCXX %s
// CHECK-NOSTDINCXX: "-cc1"
// CHECK-NOSTDINCXX-NOT: "-internal-isystem" "{{[^"]*}}c++{{/|\\\\}}v1"

// RUN: %clangxx -### %s --target=x86_64-unknown-toyos -nostdlib++ \
// RUN:     --sysroot=%S/Inputs/basic_toyos_tree 2>&1 \
// RUN:     | FileCheck --check-prefix=CHECK-NOSTDLIBXX %s
// CHECK-NOSTDLIBXX: {{.*}}ld.lld{{.*}}"
// CHECK-NOSTDLIBXX-NOT: "-lc++"
// CHECK-NOSTDLIBXX-SAME: "-ltoyos_c"
