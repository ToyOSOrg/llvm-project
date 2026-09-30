//===--- ToyOS.cpp - ToyOS ToolChain Implementations ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ToyOS.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Path.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

void tools::toyos::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                        const InputInfo &Output,
                                        const InputInfoList &Inputs,
                                        const ArgList &Args,
                                        const char *LinkingOutput) const {
  const auto &ToolChain = static_cast<const toolchains::ToyOS &>(getToolChain());
  const Driver &D = ToolChain.getDriver();
  ArgStringList CmdArgs;

  // Silence warning for "clang -g foo.o -o foo"
  Args.ClaimAllArgs(options::OPT_g_Group);
  // and "clang -emit-llvm foo.o -o foo"
  Args.ClaimAllArgs(options::OPT_emit_llvm);
  // and for "clang -w foo.o -o foo". Other warning options are already
  // handled somewhere else.
  Args.ClaimAllArgs(options::OPT_w);
  // and for "clang -nostartfiles": there is no start file to leave out, because
  // the C library carries the entry point.
  Args.ClaimAllArgs(options::OPT_nostartfiles);

  const bool IsRelocatable = Args.hasArg(options::OPT_r);
  const bool IsShared = Args.hasArg(options::OPT_shared);

  if (!D.SysRoot.empty())
    CmdArgs.push_back(Args.MakeArgString("--sysroot=" + D.SysRoot));

  // ToyOS loads every executable as a position-independent image and applies
  // its relocations itself, so there is no program interpreter to name.
  if (!IsShared && !IsRelocatable)
    CmdArgs.push_back("-pie");

  if (IsRelocatable) {
    CmdArgs.push_back("-r");
  } else {
    CmdArgs.push_back("--eh-frame-hdr");
    if (Args.hasArg(options::OPT_static))
      CmdArgs.push_back("-Bstatic");
    if (IsShared) {
      CmdArgs.push_back("-shared");
      // A shared object binds its own definitions: ToyOS has no symbol
      // interposition.
      CmdArgs.push_back("-Bsymbolic");
    }
    if (Args.hasArg(options::OPT_rdynamic))
      CmdArgs.push_back("-export-dynamic");
  }

  if (Args.hasArg(options::OPT_s))
    CmdArgs.push_back("-s");

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  Args.addAllArgs(CmdArgs, {options::OPT_L, options::OPT_u});

  ToolChain.AddFilePathLibArgs(Args, CmdArgs);

  if (D.isUsingLTO())
    addLTOOptions(ToolChain, Args, CmdArgs, Output, Inputs,
                  D.getLTOMode() == LTOK_Thin);

  addLinkerCompressDebugSectionsOption(ToolChain, Args, CmdArgs);
  AddLinkerInputs(ToolChain, Inputs, Args, CmdArgs, JA);

  // Sampled first so that it is claimed even under -nostdlib.
  const bool NoLibc = Args.hasArg(options::OPT_nolibc);
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs) &&
      !IsRelocatable) {
    if (D.CCCIsCXX() && ToolChain.ShouldLinkCXXStdlib(Args))
      ToolChain.AddCXXStdlibLibArgs(Args, CmdArgs);

    // The C library carries the program's entry point and the compiler's
    // runtime builtins itself, so there is no start file and no separate
    // runtime library to link.
    if (!NoLibc)
      CmdArgs.push_back("-ltoyos_c");
  }

  const char *Exec = Args.MakeArgString(ToolChain.GetLinkerPath());
  C.addCommand(std::make_unique<Command>(JA, *this,
                                         ResponseFileSupport::AtFileCurCP(),
                                         Exec, CmdArgs, Inputs, Output));
}

ToyOS::ToyOS(const Driver &D, const llvm::Triple &Triple, const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  getProgramPaths().push_back(getDriver().Dir);

  if (!D.SysRoot.empty()) {
    SmallString<128> P(D.SysRoot);
    llvm::sys::path::append(P, "lib");
    getFilePaths().push_back(std::string(P));
  }
}

Tool *ToyOS::buildLinker() const { return new tools::toyos::Linker(*this); }

void ToyOS::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                      ArgStringList &CC1Args) const {
  const Driver &D = getDriver();

  if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  if (!DriverArgs.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> P(D.ResourceDir);
    llvm::sys::path::append(P, "include");
    addSystemInclude(DriverArgs, CC1Args, P);
  }

  if (DriverArgs.hasArg(options::OPT_nostdlibinc) || D.SysRoot.empty())
    return;

  SmallString<128> P(D.SysRoot);
  llvm::sys::path::append(P, "include");
  addExternCSystemInclude(DriverArgs, CC1Args, P);
}

void ToyOS::AddClangCXXStdlibIncludeArgs(const ArgList &DriverArgs,
                                         ArgStringList &CC1Args) const {
  const Driver &D = getDriver();

  if (DriverArgs.hasArg(options::OPT_nostdinc, options::OPT_nostdlibinc,
                        options::OPT_nostdincxx) ||
      D.SysRoot.empty())
    return;

  // libc++ is the C++ standard library ToyOS has. A sysroot is one target's,
  // so its headers, __config_site among them, are in include/c++/<version>.
  if (GetCXXStdlibType(DriverArgs) != ToolChain::CST_Libcxx)
    return;

  SmallString<128> P(D.SysRoot);
  llvm::sys::path::append(P, "include");
  std::string Version = detectLibcxxVersion(P);
  if (Version.empty())
    return;
  llvm::sys::path::append(P, "c++", Version);
  addSystemInclude(DriverArgs, CC1Args, P);
}
