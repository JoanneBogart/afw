// -*- lsst-c++ -*-
/**
 * \file
 * \brief A boolean pair class used to express the output of spatial predicates on Point and Extent.
 */
#ifndef LSST_AFW_GEOM_COORDINATEEXPR_H
#define LSST_AFW_GEOM_COORDINATEEXPR_H

#include "lsst/afw/geom/CoordinateBase.h"

namespace lsst { namespace afw { namespace geom {

/**
 *  \brief A boolean coordinate.
 *
 *  CoordinateExpr is intended to be used as a temporary in coordinate comparisons:
 *  \code
 *  Point2D a(3.5,1.2);
 *  Point2D b(-1.5,4.3);
 *  std::cout << all(a < b) << std::endl;  // false
 *  std::cout << any(a < b) << std::endl;  // true
 *  \endcode
 *  
 *  CoordinateExpr is not a true lazy-evaluation expression template, as that seems unnecessary when
 *  the object is typically  only two bools large (smaller than the raw pointers necessary to implement
 *  a lazy solution).  The consequence is that there's no short-circuiting of logical operators, but I don't
 *  think that will even remotely matter for most use cases.  The any() and all() functions do support
 *  short-circuiting.
 */
template<int N>
class CoordinateExpr : public CoordinateBase<CoordinateExpr<N>,bool,N> {
    typedef CoordinateBase<CoordinateExpr<N>,bool,N> Super;
public:

    /**
     *  \brief Constructors
     *
     *  See the CoordinateBase constructors for more discussion.
     */
    //@{
    explicit CoordinateExpr(bool val=false) : Super(val) {}

    template <typename Vector>
    explicit CoordinateExpr(Eigen::MatrixBase<Vector> const & vector) : Super(vector) {}
    //@}

    /**
     *  @name Logical operators
     *
     *  These operators do not provide interoperability with scalars.
     */
    //@{
    CoordinateExpr operator&&(CoordinateExpr const & rhs) const;
    CoordinateExpr operator||(CoordinateExpr const & rhs) const;
    CoordinateExpr operator!() const;
    //@}

};

/// \brief Return true if all elements are true.
template <int N>
inline bool all(CoordinateExpr<N> const & expr) {
    for (register int n=0; n<N; ++n) if (expr[n]) return false;
    return true;
}

/// \brief Return true if any elements are true.
template <int N>
inline bool any(CoordinateExpr<N> const & expr) {
    for (register int n=0; n<N; ++n) if (expr[n]) return true;
    return false;
}

}}}

#endif
