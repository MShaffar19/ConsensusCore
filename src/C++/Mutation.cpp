// Author: Patrick Marks, David Alexander

#include <ConsensusCore/Mutation.hpp>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cassert>
#include <string>
#include <vector>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <ConsensusCore/Align/PairwiseAlignment.hpp>
#include <ConsensusCore/Types.hpp>
#include <ConsensusCore/Utils.hpp>

using std::max;

namespace ConsensusCore {
std::string Mutation::ToString() const
{
    using boost::str;
    using boost::format;

    switch (Type()) {
        case INSERTION:
            return str(format("Insertion (%s) @%d") % newBases_ % start_);
        case DELETION:
            return str(format("Deletion @%d:%d") % start_ % end_);
        case SUBSTITUTION:
            return str(format("Substitution (%s) @%d:%d") % newBases_ % start_ % end_);
        default:
            ShouldNotReachHere();
    }
}

std::ostream& operator<<(std::ostream& out, const Mutation& m)
{
    out << m.ToString();
    return out;
}

ScoredMutation Mutation::WithScore(float score) const { return ScoredMutation(*this, score); }

static void _ApplyMutationInPlace(const Mutation& mut, int start, std::string* tpl)
{
    if (mut.IsSubstitution()) {
        (*tpl).replace(start, mut.End() - mut.Start(), mut.NewBases());
    } else if (mut.IsDeletion()) {
        (*tpl).erase(start, mut.End() - mut.Start());
    } else if (mut.IsInsertion()) {
        (*tpl).insert(start, mut.NewBases());
    }
}

std::string ApplyMutation(const Mutation& mut, const std::string& tpl)
{
    std::string tplCopy(tpl);
    _ApplyMutationInPlace(mut, mut.Start(), &tplCopy);
    return tplCopy;
}

std::string ApplyMutations(const std::vector<Mutation>& muts, const std::string& tpl)
{
    std::string tplCopy(tpl);
    std::vector<Mutation> sortedMuts(muts);
    std::sort(sortedMuts.begin(), sortedMuts.end());
    int runningLengthDiff = 0;
    foreach (const Mutation& mut, sortedMuts) {
        _ApplyMutationInPlace(mut, mut.Start() + runningLengthDiff, &tplCopy);
        runningLengthDiff += mut.LengthDiff();
    }
    return tplCopy;
}

std::string MutationsToTranscript(const std::vector<Mutation>& mutations, const std::string& tpl)
{
    std::vector<Mutation> sortedMuts(mutations);
    std::sort(sortedMuts.begin(), sortedMuts.end());

    // Build an alignnment transcript corresponding to these mutations.
    int tpos = 0;
    std::string transcript = "";
    foreach (const Mutation& m, sortedMuts) {
        for (; tpos < m.Start(); ++tpos) {
            transcript.push_back('M');
        }

        if (m.IsInsertion()) {
            transcript += std::string(m.LengthDiff(), 'I');
        } else if (m.IsDeletion()) {
            transcript += std::string(-m.LengthDiff(), 'D');
            tpos += -m.LengthDiff();
        } else if (m.IsSubstitution()) {
            int len = m.End() - m.Start();
            transcript += std::string(len, 'R');
            tpos += len;
        } else {
            ShouldNotReachHere();
        }
    }
    for (; tpos < static_cast<int>(tpl.length()); ++tpos) {
        transcript.push_back('M');
    }
    return transcript;
}

// MutatedTemplatePositions:
//  * Returns a vector of length (tpl.length()+1), which, roughly speaking,
//    indicates the positions in the mutated template tpl' of the characters
//    in tpl.
//  * More precisely, for any slice [s, e) of tpl, letting:
//      - t[s, e) denote the induced substring of the template;
//      - m[s, e) denote the subvector of mutations with Position
//        in [s, e);
//      - t' denote the mutated template; and
//      - t[s, e)' denote the result of applying mutation m[s, e) to t[s, e),
//    the resultant vector mtp satisfies t'[mtp[s], mtp[e]) == t[s,e)'.
//  * Example:
//               01234567                           0123456
//              "GATTACA" -> (Del T@2, Ins C@5) -> "GATACCA";
//    here mtp = 01223567, which makes sense, because for instance:
//      - t[0,3)=="GAT" has become t'[0,2)=="GA";
//      - t[0,2)=="GA"  remains "GA"==t'[0,2);
//      - t[4,7)=="ACA" has become t[3,7)=="ACCA",
//      - t[5,7)=="CA"  remains "CA"==t'[5,7).
//
std::vector<int> TargetToQueryPositions(const std::vector<Mutation>& mutations,
                                        const std::string& tpl)
{
    return TargetToQueryPositions(MutationsToTranscript(mutations, tpl));
}

ScoredMutation::ScoredMutation(const Mutation& m, float score) : Mutation(m), score_(score) {}

ScoredMutation::ScoredMutation() : Mutation(), score_(0) {}

float ScoredMutation::Score() const { return score_; }

std::string ScoredMutation::ToString() const
{
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream& operator<<(std::ostream& out, const ScoredMutation& m)
{
    out << m.Mutation::ToString() << " " << boost::format("%0.2f") % m.Score();
    return out;
}
}
