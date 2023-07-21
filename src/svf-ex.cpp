//===- svf-ex.cpp -- A driver example of SVF-------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===-----------------------------------------------------------------------===//

/*
 // A driver program of SVF including usages of SVF APIs
 //
 // Author: Yulei Sui,
 */

#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"
#include "sstream"
#include "fstream"

using namespace SVF;
using namespace SVFUtil;
using namespace std;

void computePts(SVFIR *svfir, Andersen *ander, const string &moduleName) {

    OrderedMap<u32_t, u32_t> ptsSzToNum;
    for (const auto &item: *svfir) {
        u32_t ct = ander->getPts(item.first).count();
        if (ct > 0) {
            if (!ptsSzToNum.count(ct)) ptsSzToNum[ct] = 0;
            ptsSzToNum[ct]++;
        }
    }
    std::string str;
    stringstream ss(str);
    for (const auto &it: ptsSzToNum) {
        ss << it.first << "," << it.second << "\n";
    }
    cout << ss.str();

    // Open a file in output mode (std::ios::out)
    std::fstream file(moduleName + ".pts", std::ios::out);

    if (!file.is_open()) {
        std::cout << "Error opening the file!" << std::endl;
        return;
    }

    // Write data to the file
    file << ss.str();

    // Close the file
    file.close();

    std::cout << "Data has been written to " << moduleName + ".pts" << std::endl;
}

int main(int argc, char **argv) {
    std::vector<std::string> moduleNameVec;
    moduleNameVec = OptionBase::parseOptions(
            argc, argv, "SVF Example", "[options] <input-bitcode...>"
    );

    SVFModule *svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR *pag = builder.build();

    /// Create Andersen's pointer analysis
    Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    /// Call Graph
    PTACallGraph *callgraph = ander->getPTACallGraph();

    /// ICFG
    ICFG *icfg = pag->getICFG();
    icfg->updateCallGraph(callgraph);
    builder.updateCallGraph(callgraph);

    computePts(pag, ander, svfModule->getModuleIdentifier());

    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    SVF::LLVMModuleSet::releaseLLVMModuleSet();
    llvm::llvm_shutdown();
    return 0;
}

