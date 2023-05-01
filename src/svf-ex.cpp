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

using namespace SVF;
using namespace SVFUtil;


/*!
 * Given an actual out/ret SVFGNode, compute its reachable actual in/param SVFGNodes
 * @param src
 * @param outToIns 
 */
void computeOutToIns(const SVFGNode *src, Map<const SVFGNode *, Set<const SVFGNode *>> &outToIns) {
    if(outToIns.count(src)) return;

    FIFOWorkList<const SVFGNode*> workList;
    Set<const SVFGNode*> visited;

    u32_t callSiteID;
    Set<const SVFGNode*> ins;
    for (const auto &inEdge: src->getInEdges()) {
        workList.push(inEdge->getSrcNode());
        visited.insert(inEdge->getSrcNode());
        if (const RetDirSVFGEdge *retEdge = SVFUtil::dyn_cast<RetDirSVFGEdge>(inEdge)) {
            callSiteID = retEdge->getCallSiteId();
        } else if (const RetIndSVFGEdge *retEdge = SVFUtil::dyn_cast<RetIndSVFGEdge>(inEdge)) {
            callSiteID = retEdge->getCallSiteId();
        } else {
            assert(false && "return edge does not have a callsite ID?");
        }
    }

    while (!workList.empty()) {
        const SVFGNode *cur = workList.pop();
        if (SVFUtil::isa<FormalINSVFGNode>(cur) || SVFUtil::isa<FormalParmVFGNode>(cur)) {
            for (const auto &inEdge: cur->getInEdges()) {
                if (const CallDirSVFGEdge *callEdge = SVFUtil::dyn_cast<CallDirSVFGEdge>(inEdge)) {
                    if (callSiteID == callEdge->getCallSiteId()) ins.insert(callEdge->getSrcNode());
                } else if (const CallIndSVFGEdge *callEdge = SVFUtil::dyn_cast<CallIndSVFGEdge>(inEdge)) {
                    if (callSiteID == callEdge->getCallSiteId()) ins.insert(callEdge->getSrcNode());
                } else {
                    assert(false && "call edge does not have a callsite ID?");
                }
            }
            continue;
        }
        if (SVFUtil::isa<ActualOUTSVFGNode>(cur) || SVFUtil::isa<ActualRetVFGNode>(cur)) {
            computeOutToIns(cur, outToIns);
            for (const auto &in: outToIns[cur]) {
                if (!visited.count(in)) {
                    visited.insert(in);
                    workList.push(in);
                }
            }
            continue;
        }
        for (const auto &inEdge: cur->getInEdges()) {
            if (inEdge->getSrcNode()->getFun() == cur->getFun() && !visited.count(inEdge->getSrcNode())) {
                visited.insert(inEdge->getSrcNode());
                workList.push(inEdge->getSrcNode());
            }
        }
    }
    outToIns[src] = ins;

}
int main(int argc, char ** argv)
{
    std::vector<std::string> moduleNameVec;
    moduleNameVec = OptionBase::parseOptions(
            argc, argv, "SVF Example", "[options] <input-bitcode...>"
    );

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR* pag = builder.build();

    /// Create Andersen's pointer analysis
    Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    /// Call Graph
    PTACallGraph* callgraph = ander->getPTACallGraph();

    /// ICFG
    ICFG* icfg = pag->getICFG();

    /// Sparse value-flow graph (SVFG)
    SVFGBuilder svfBuilder;
    SVFG* svfg = svfBuilder.buildFullSVFG(ander);
    svfg->dump("SVFG");

    Map<const SVFGNode*, Set<const SVFGNode*>> outToIns;

    for (const auto &it: *svfg) {
        if (SVFUtil::isa<ActualOUTSVFGNode>(it.second) || SVFUtil::isa<ActualRetVFGNode>(it.second)) {
            computeOutToIns(it.second, outToIns);
        }
    }

    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    SVF::LLVMModuleSet::releaseLLVMModuleSet();
    llvm::llvm_shutdown();
    return 0;
}

