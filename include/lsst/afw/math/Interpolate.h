// -*- LSST-C++ -*-
#if !defined(LSST_AFW_MATH_INTERPOLATE_H)
#define LSST_AFW_MATH_INTERPOLATE_H

/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
 * 
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the LSST License Statement and 
 * the GNU General Public License along with this program.  If not, 
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */
#include "lsst/base.h"

namespace lsst {
namespace afw {
namespace math {

class InterpolateControl;

 /**
  * @brief Interpolate values for a set of x,y vector<>s
  * @ingroup afw
  * @author Steve Bickerton
  */
class Interpolate {
public:
    enum Style {
        UNKNOWN = -1,
        CONSTANT = 0,
        LINEAR,
        NATURAL_SPLINE,
        CUBIC_SPLINE,
        CUBIC_SPLINE_PERIODIC,
        AKIMA_SPLINE,
        AKIMA_SPLINE_PERIODIC,
        TAUT_SPLINE,
        NUM_STYLES
    };

    friend PTR(Interpolate) makeInterpolate(std::vector<double> const &x, std::vector<double> const &y,
                                            Interpolate::Style const style);
    friend PTR(Interpolate) makeInterpolate(std::vector<double> const &x, std::vector<double> const &y,
                                            InterpolateControl const& ictrl);
    
    virtual ~Interpolate() {}
    virtual double interpolate(double const x) const = 0;
    virtual void interpolate(std::vector<double> const& x, std::vector<double> &y) const;
    virtual double derivative(double const x) const = 0;
    virtual void derivative(std::vector<double>  const& x, std::vector<double> &dydx) const;
    virtual std::vector<double> roots(double const value, double const x0, double const x1) const;
protected:
    /**
     * Base class ctor
     */
    Interpolate(std::vector<double> const &x, ///< the ordinates of points
                std::vector<double> const &y, ///< the values at x[]
                Interpolate::Style const style=UNKNOWN ///< desired interpolator
               ) : _x(x), _y(y), _style(style) {}
    Interpolate(std::pair<std::vector<double>, std::vector<double> > const xy,
                Interpolate::Style const style=UNKNOWN);

    std::vector<double> const _x;
    std::vector<double> const _y;
    Interpolate::Style const _style;
private:
    Interpolate(Interpolate const&);
    Interpolate& operator=(Interpolate const&);
};

/**
 * \brief Control interpolation
 *
 * A base class to pass to makeInterpolate;  subclass if you have real information to share
 */
class InterpolateControl {
public:
    explicit InterpolateControl(Interpolate::Style style) : _style(style) {}
    virtual ~InterpolateControl() {}
    Interpolate::Style getStyle() const { return _style; }
private:
    Interpolate::Style _style;
};

PTR(Interpolate) makeInterpolate(std::vector<double> const &x, std::vector<double> const &y,
                                 Interpolate::Style const style=Interpolate::AKIMA_SPLINE);
PTR(Interpolate) makeInterpolate(std::vector<double> const &x, std::vector<double> const &y,
                                 InterpolateControl const& ictrl);
    
Interpolate::Style stringToInterpStyle(std::string const &style);
Interpolate::Style lookupMaxInterpStyle(int const n);
int lookupMinInterpPoints(Interpolate::Style const style);
        
}}}
                     
#endif // LSST_AFW_MATH_INTERPOLATE_H
