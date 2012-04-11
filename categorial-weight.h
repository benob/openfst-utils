// categorial-weight.h

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2012 Aix-Marseille Univ.
// Author: benoit.favre@lif.univ-mrs.fr (Benoit Favre)
//
// \file
// Categorial weight set and associated semiring operation definitions.

#ifndef FST_LIB_CATEGORIAL_WEIGHT_H__
#define FST_LIB_CATEGORIAL_WEIGHT_H__

#include <list>
#include <string>

#include <fst/product-weight.h>
#include <fst/weight.h>

namespace fst {

    const int kCategorialInfinity = -1;      // Label for the infinite string
    const int kCategorialBad = -2;           // Label for a non-string
    const char kCategorialSeparator = '_';   // Label separator in strings
    const int kCategorialLeftBracket = -3;
    const int kCategorialRightBracket = -4;
    const int kCategorialLeftDiv = -5;
    const int kCategorialRightDiv = -6;

    // Determines whether to use left or right categorial semiring.  Includes
    // restricted versions that signal an error if proper prefixes
    // (suffixes) would otherwise be returned by Plus, useful with various
    // algorithms that require functional transducer input with the
    // categorial semirings.
    enum CategorialType { CATEGORIAL_LEFT = 0, CATEGORIAL_RIGHT = 1 ,
        CATEGORIAL_LEFT_RESTRICT = 2, CATEGORIAL_RIGHT_RESTRICT };

#define REVERSE_CATEGORIAL_TYPE(S)                                  \
    ((S) == CATEGORIAL_LEFT ? CATEGORIAL_RIGHT :                         \
     ((S) == CATEGORIAL_RIGHT ? CATEGORIAL_LEFT :                        \
      ((S) == CATEGORIAL_LEFT_RESTRICT ? CATEGORIAL_RIGHT_RESTRICT :     \
       CATEGORIAL_LEFT_RESTRICT)))

    template <typename L, CategorialType S = CATEGORIAL_LEFT>
        class CategorialWeight;

    template <typename L, CategorialType S = CATEGORIAL_LEFT>
        class CategorialWeightIterator;

    template <typename L, CategorialType S = CATEGORIAL_LEFT>
        class CategorialWeightReverseIterator;

    template <typename L, CategorialType S>
        bool operator==(const CategorialWeight<L, S> &,  const CategorialWeight<L, S> &);


    // Categorial semiring: (longest_common_prefix/suffix, ., Infinity, Epsilon)
    template <typename L, CategorialType S>
        class CategorialWeight {
            public:
                typedef L Label;
                typedef CategorialWeight<L, REVERSE_CATEGORIAL_TYPE(S)> ReverseWeight;

                friend class CategorialWeightIterator<L, S>;
                friend class CategorialWeightReverseIterator<L, S>;
                friend bool operator==<>(const CategorialWeight<L, S> &,
                        const CategorialWeight<L, S> &);

                CategorialWeight() { Init(); }

                template <typename Iter>
                    CategorialWeight(const Iter &begin, const Iter &end) {
                        Init();
                        for (Iter iter = begin; iter != end; ++iter)
                            PushBack(*iter);
                    }

                explicit CategorialWeight(L l) { Init(); PushBack(l); }

                static const CategorialWeight<L, S> &Zero() {
                    static const CategorialWeight<L, S> zero(kCategorialInfinity);
                    return zero;
                }

                static const CategorialWeight<L, S> &One() {
                    static const CategorialWeight<L, S> one;
                    return one;
                }

                static const string &Type() {
                    static const string type =
                        S == CATEGORIAL_LEFT ? "categorial" :
                        (S == CATEGORIAL_RIGHT ? "right_categorial" :
                         (S == CATEGORIAL_LEFT_RESTRICT ? "restricted_categorial" :
                          "right_restricted_categorial"));
                    return type;
                }

                bool Member() const;

                istream &Read(istream &strm);

                ostream &Write(ostream &strm) const;

                size_t Hash() const;

                CategorialWeight<L, S> Quantize(float delta = kDelta) const {
                    return *this;
                }

                ReverseWeight Reverse() const;

                static uint64 Properties() {
                    return (S == CATEGORIAL_LEFT || S == CATEGORIAL_LEFT_RESTRICT ?
                            kLeftSemiring : kRightSemiring) | kIdempotent | kPath;
                }

                // NB: This needs to be uncommented only if default fails for this impl.
                // CategorialWeight<L, S> &operator=(const CategorialWeight<L, S> &w);

                // These operations combined with the CategorialWeightIterator and
                // CategorialWeightReverseIterator provide the access and mutation of
                // the string internal elements.

                // Common initializer among constructors.
                void Init() { first_ = 0; }

                // Clear existing CategorialWeight.
                void Clear() { first_ = 0; rest_.clear(); }

                size_t Size() const { return first_ ? rest_.size() + 1 : 0; }

                void PushFront(L l) {
                    if (first_)
                        rest_.push_front(first_);
                    first_ = l;
                }

                void PushBack(L l) {
                    if (!first_)
                        first_ = l;
                    else
                        rest_.push_back(l);
                }

            private:
                L first_;         // first label in string (0 if empty)
                list<L> rest_;    // remaining labels in string
        };


    // Traverses string in forward direction.
    template <typename L, CategorialType S>
        class CategorialWeightIterator {
            public:
                explicit CategorialWeightIterator(const CategorialWeight<L, S>& w)
                    : first_(w.first_), rest_(w.rest_), init_(true),
                    iter_(rest_.begin()) {}

                bool Done() const {
                    if (init_) return first_ == 0;
                    else return iter_ == rest_.end();
                }

                const L& Value() const { return init_ ? first_ : *iter_; }

                void Next() {
                    if (init_) init_ = false;
                    else  ++iter_;
                }

                void Reset() {
                    init_ = true;
                    iter_ = rest_.begin();
                }

            private:
                const L &first_;
                const list<L> &rest_;
                bool init_;   // in the initialized state?
                typename list<L>::const_iterator iter_;

                DISALLOW_COPY_AND_ASSIGN(CategorialWeightIterator);
        };


    // Traverses string in backward direction.
    template <typename L, CategorialType S>
        class CategorialWeightReverseIterator {
            public:
                explicit CategorialWeightReverseIterator(const CategorialWeight<L, S>& w)
                    : first_(w.first_), rest_(w.rest_), fin_(first_ == 0),
                    iter_(rest_.rbegin()) {}

                bool Done() const { return fin_; }

                const L& Value() const { return iter_ == rest_.rend() ? first_ : *iter_; }

                void Next() {
                    if (iter_ == rest_.rend()) fin_ = true;
                    else  ++iter_;
                }

                void Reset() {
                    fin_ = false;
                    iter_ = rest_.rbegin();
                }

            private:
                const L &first_;
                const list<L> &rest_;
                bool fin_;   // in the final state?
                typename list<L>::const_reverse_iterator iter_;

                DISALLOW_COPY_AND_ASSIGN(CategorialWeightReverseIterator);
        };


    // CategorialWeight member functions follow that require
    // CategorialWeightIterator or CategorialWeightReverseIterator.

    template <typename L, CategorialType S>
        inline istream &CategorialWeight<L, S>::Read(istream &strm) {
            Clear();
            int32 size;
            ReadType(strm, &size);
            for (int i = 0; i < size; ++i) {
                L label;
                ReadType(strm, &label);
                PushBack(label);
            }
            return strm;
        }

    template <typename L, CategorialType S>
        inline ostream &CategorialWeight<L, S>::Write(ostream &strm) const {
            int32 size =  Size();
            WriteType(strm, size);
            for (CategorialWeightIterator<L, S> iter(*this); !iter.Done(); iter.Next()) {
                L label = iter.Value();
                WriteType(strm, label);
            }
            return strm;
        }

    template <typename L, CategorialType S>
        inline bool CategorialWeight<L, S>::Member() const {
            if (Size() != 1)
                return true;
            CategorialWeightIterator<L, S> iter(*this);
            return iter.Value() != kCategorialBad;
        }

    template <typename L, CategorialType S>
        inline typename CategorialWeight<L, S>::ReverseWeight
        CategorialWeight<L, S>::Reverse() const {
            ReverseWeight rw;
            for (CategorialWeightIterator<L, S> iter(*this); !iter.Done(); iter.Next())
                rw.PushFront(iter.Value());
            return rw;
        }

    template <typename L, CategorialType S>
        inline size_t CategorialWeight<L, S>::Hash() const {
            size_t h = 0;
            for (CategorialWeightIterator<L, S> iter(*this); !iter.Done(); iter.Next())
                h ^= h<<1 ^ iter.Value();
            return h;
        }

    // NB: This needs to be uncommented only if default fails for this the impl.
    //
    // template <typename L, CategorialType S>
    // inline CategorialWeight<L, S>
    // &CategorialWeight<L, S>::operator=(const CategorialWeight<L, S> &w) {
    //   if (this != &w) {
    //     Clear();
    //     for (CategorialWeightIterator<L, S> iter(w); !iter.Done(); iter.Next())
    //       PushBack(iter.Value());
    //   }
    //   return *this;
    // }

    template <typename L, CategorialType S>
        inline bool operator==(const CategorialWeight<L, S> &w1,
                const CategorialWeight<L, S> &w2) {
            if (w1.Size() != w2.Size())
                return false;

            CategorialWeightIterator<L, S> iter1(w1);
            CategorialWeightIterator<L, S> iter2(w2);

            for (; !iter1.Done() ; iter1.Next(), iter2.Next())
                if (iter1.Value() != iter2.Value())
                    return false;

            return true;
        }

    template <typename L, CategorialType S>
        inline bool operator!=(const CategorialWeight<L, S> &w1,
                const CategorialWeight<L, S> &w2) {
            return !(w1 == w2);
        }

    template <typename L, CategorialType S>
        inline bool ApproxEqual(const CategorialWeight<L, S> &w1,
                const CategorialWeight<L, S> &w2,
                float delta = kDelta) {
            return w1 == w2;
        }

    template <typename L, CategorialType S>
        inline ostream &operator<<(ostream &strm, const CategorialWeight<L, S> &w) {
            CategorialWeightIterator<L, S> iter(w);
            bool needSeparator = false;
            if (iter.Done())
                return strm << "Epsilon";
            else if (iter.Value() == kCategorialInfinity)
                return strm << "Infinity";
            else if (iter.Value() == kCategorialBad)
                return strm << "BadCategorial";
            else
                for (size_t i = 0; !iter.Done(); ++i, iter.Next()) {
                    if (iter.Value() == kCategorialLeftBracket) { strm << '<'; needSeparator = false; }
                    else if (iter.Value() == kCategorialRightBracket) { strm << '>'; needSeparator = false; }
                    else if (iter.Value() == kCategorialLeftDiv) { strm << '\\'; needSeparator = false; }
                    else {
                        if (needSeparator)
                            strm << kCategorialSeparator;
                        strm << iter.Value();
                        needSeparator = true;
                    }
                }
            return strm;
        }

    template <typename L, CategorialType S>
        inline istream &operator>>(istream &strm, CategorialWeight<L, S> &w) {
            string s;
            strm >> s;
            if (s == "Infinity") {
                w = CategorialWeight<L, S>::Zero();
            } else if (s == "Epsilon") {
                w = CategorialWeight<L, S>::One();
            } else {
                w.Clear();
                char *p = 0;
                for (const char *cs = s.c_str(); !p || *p != '\0'; cs = p + 1) {
                    int l = strtoll(cs, &p, 10);
                    if (p == cs || (*p != 0 && *p != kCategorialSeparator)) {
                        strm.clear(std::ios::badbit);
                        break;
                    }
                    w.PushBack(l);
                }
            }
            return strm;
        }


    // Default is for the restricted left and right semirings.  Categorial
    // equality is required (for non-Zero() input. This restriction
    // is used in e.g. Determinize to ensure functional input.
    template <typename L, CategorialType S>  inline CategorialWeight<L, S>
        Plus(const CategorialWeight<L, S> &w1,
                const CategorialWeight<L, S> &w2) {
            if (w1 == CategorialWeight<L, S>::Zero())
                return w2;
            if (w2 == CategorialWeight<L, S>::Zero())
                return w1;

            if (w1 != w2)
                LOG(FATAL) << "CategorialWeight::Plus: unequal arguments "
                    << "(non-functional FST?)";

            return w1;
        }


    // Longest common prefix for left categorial semiring.
    template <typename L>  inline CategorialWeight<L, CATEGORIAL_LEFT>
        Plus(const CategorialWeight<L, CATEGORIAL_LEFT> &w1,
                const CategorialWeight<L, CATEGORIAL_LEFT> &w2) {
            if (w1 == CategorialWeight<L, CATEGORIAL_LEFT>::Zero())
                return w2;
            if (w2 == CategorialWeight<L, CATEGORIAL_LEFT>::Zero())
                return w1;

            CategorialWeightIterator<L, CATEGORIAL_LEFT> iter1(w1);
            CategorialWeightIterator<L, CATEGORIAL_LEFT> iter2(w2);
            for (; !iter1.Done() && !iter2.Done(); iter1.Next(), iter2.Next()) {
                if(iter1.Value() < iter2.Value()) return w1;
                else if(iter1.Value() > iter2.Value()) return w2;
            }
            if(!iter2.Done()) return w1;
            return w2;
        }


    // Longest common suffix for right categorial semiring.
    template <typename L>  inline CategorialWeight<L, CATEGORIAL_RIGHT>
        Plus(const CategorialWeight<L, CATEGORIAL_RIGHT> &w1,
                const CategorialWeight<L, CATEGORIAL_RIGHT> &w2) {
            if (w1 == CategorialWeight<L, CATEGORIAL_RIGHT>::Zero())
                return w2;
            if (w2 == CategorialWeight<L, CATEGORIAL_RIGHT>::Zero())
                return w1;

            CategorialWeightIterator<L, CATEGORIAL_RIGHT> iter1(w1);
            CategorialWeightIterator<L, CATEGORIAL_RIGHT> iter2(w2);
            for (; !iter1.Done() && !iter2.Done(); iter1.Next(), iter2.Next()) {
                if(iter1.Value() < iter2.Value()) return w1;
                else if(iter1.Value() > iter2.Value()) return w2;
            }
            if(!iter2.Done()) return w1;
            return w2;
        }


    template <typename L, CategorialType S>
        inline CategorialWeight<L, S> Times(const CategorialWeight<L, S> &w1,
                const CategorialWeight<L, S> &w2) {
            if (w1 == CategorialWeight<L, S>::Zero() || w2 == CategorialWeight<L, S>::Zero())
                return CategorialWeight<L, S>::Zero();

            CategorialWeight<L, S> prod(w1);
            if(w1.Size() > 0) {
                //prod.PushFront(kCategorialLeftBracket);
                //prod.PushBack(kCategorialRightBracket);
            }
            ////if(w2.Size() > 0) prod.PushBack(kCategorialLeftBracket);
            for (CategorialWeightIterator<L, S> iter(w2); !iter.Done(); iter.Next())
                prod.PushBack(iter.Value());
            ////if(w2.Size() > 0) prod.PushBack(kCategorialRightBracket);
            /*cerr << "TIMES ";
            cerr << w1 << " ";
            cerr << w2 << " ";
            cerr << prod << endl;*/

            return prod;
        }


    // Default is for left division in the left categorial and the
    // left restricted categorial semirings.
    template <typename L, CategorialType S> inline CategorialWeight<L, S>
        Divide(const CategorialWeight<L, S> &w1,
                const CategorialWeight<L, S> &w2,
                DivideType typ) {

            if (typ != DIVIDE_LEFT)
                LOG(FATAL) << "CategorialWeight::Divide: only left division is defined "
                    << "for the " << CategorialWeight<L, S>::Type() << " semiring";

            if (w2 == CategorialWeight<L, S>::Zero())
                return CategorialWeight<L, S>(kCategorialBad);
            else if (w1 == CategorialWeight<L, S>::Zero())
                return CategorialWeight<L, S>::Zero();

            if(w1 == w2) return CategorialWeight<L, S>::One();
            CategorialWeight<L, S> div;
            CategorialWeightIterator<L, S> iter1(w1);
            CategorialWeightIterator<L, S> iter2(w2);
            bool needsBrackets = false;
            for (; !iter2.Done(); iter2.Next()) {
                if(iter2.Value() == kCategorialLeftDiv) {
                    needsBrackets = true;
                    break;
                }
            }
            iter2.Reset();
            if(needsBrackets) div.PushBack(kCategorialLeftBracket);
            for (; !iter2.Done(); iter2.Next()) {
                div.PushBack(iter2.Value());
            }
            if(needsBrackets) div.PushBack(kCategorialRightBracket);
            div.PushBack(kCategorialLeftDiv);
            //if(w2.Size() > 0) div.PushBack(kCategorialLeftBracket);
            for (; !iter1.Done(); iter1.Next()) {
                div.PushBack(iter1.Value());
            }
            //if(w2.Size() > 0) div.PushBack(kCategorialRightBracket);
            /*cerr << "DIV ";
            cerr << w1 << " ";
            cerr << w2 << " ";
            cerr << div << endl;*/
            return div;
        }


    // Right division in the right categorial semiring.
    template <typename L> inline CategorialWeight<L, CATEGORIAL_RIGHT>
        Divide(const CategorialWeight<L, CATEGORIAL_RIGHT> &w1,
                const CategorialWeight<L, CATEGORIAL_RIGHT> &w2,
                DivideType typ) {

            //if (typ != DIVIDE_RIGHT)
                LOG(FATAL) << "CategorialWeight::Divide: only right division is defined "
                    << "for the right categorial semiring";

            if (w2 == CategorialWeight<L, CATEGORIAL_RIGHT>::Zero())
                return CategorialWeight<L, CATEGORIAL_RIGHT>(kCategorialBad);
            else if (w1 == CategorialWeight<L, CATEGORIAL_RIGHT>::Zero())
                return CategorialWeight<L, CATEGORIAL_RIGHT>::Zero();

            CategorialWeight<L, CATEGORIAL_RIGHT> div;
            CategorialWeightReverseIterator<L, CATEGORIAL_RIGHT> iter(w1);
            for (int i = 0; !iter.Done(); iter.Next(), ++i) {
                if (i >= w2.Size())
                    div.PushFront(iter.Value());
            }
            return div;
        }


    // Right division in the right restricted categorial semiring.
    template <typename L> inline CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT>
        Divide(const CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT> &w1,
                const CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT> &w2,
                DivideType typ) {

            //if (typ != DIVIDE_RIGHT)
                LOG(FATAL) << "CategorialWeight::Divide: only right division is defined "
                    << "for the right restricted categorial semiring";

            if (w2 == CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT>::Zero())
                return CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT>(kCategorialBad);
            else if (w1 == CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT>::Zero())
                return CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT>::Zero();

            CategorialWeight<L, CATEGORIAL_RIGHT_RESTRICT> div;
            CategorialWeightReverseIterator<L, CATEGORIAL_RIGHT_RESTRICT> iter(w1);
            for (int i = 0; !iter.Done(); iter.Next(), ++i) {
                if (i >= w2.Size())
                    div.PushFront(iter.Value());
            }
            return div;
        }

}  // namespace fst

#endif  // FST_LIB_CATEGORIAL_WEIGHT_H__
