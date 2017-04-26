/*
* LSST Data Management System
* See COPYRIGHT file at the top of the source tree.
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the LSST License Statement and
* the GNU General Public License along with this program. If not,
* see <http://www.lsstcorp.org/LegalNotices/>.
*/
#include "pybind11/pybind11.h"

#include <memory>

#include "astshim.h"
#include "pybind11/stl.h"
#include "numpy/arrayobject.h"
#include "ndarray/pybind11.h"

#include "lsst/afw/geom/Endpoint.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/geom/SpherePoint.h"
#include "lsst/afw/geom/Transform.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace {

// Return a string consisting of "_pythonClassName_[_fromNAxes_->_toNAxes_]",
// for example "TransformGenericToPoint3[4->3]"
template <class Class>
std::string formatStr(Class const &self, std::string const &pyClassName) {
    std::ostringstream os;
    os << pyClassName;
    auto const frameSet = self.getFrameSet();
    os << "[" << frameSet->getNin() << "->" << frameSet->getNout() << "]";
    return os.str();
}

template <class ExtraEndpoint, class FromEndpoint, class ToEndpoint, class PyClass>
void declareMethodTemplates(PyClass &cls) {
    using FirstTransform = Transform<ExtraEndpoint, FromEndpoint>;
    using SecondTransform = Transform<FromEndpoint, ToEndpoint>;
    using FinalTransform = Transform<ExtraEndpoint, ToEndpoint>;
    // Need Python-specific logic to give sensible errors for mismatched Transform types
    cls.def("_of", (FinalTransform (SecondTransform::*)(FirstTransform const &) const) &
                           SecondTransform::template of<ExtraEndpoint>,
            "first"_a);
}

// Declare Transform<FromEndpoint, ToEndpoint> using python class name TransformFrom<X>To<Y>
// where <X> and <Y> are the prefix of the from endpoint and to endpoint class, respectively,
// for example TransformFromGenericToPoint3
template <class FromEndpoint, class ToEndpoint>
void declareTransform(py::module &mod) {
    using Class = Transform<FromEndpoint, ToEndpoint>;
    using ToPoint = typename ToEndpoint::Point;
    using ToArray = typename ToEndpoint::Array;
    using FromPoint = typename FromEndpoint::Point;
    using FromArray = typename FromEndpoint::Array;

    std::string const pyClassName = "Transform" + FromEndpoint::getPrefix() + "To" + ToEndpoint::getPrefix();

    py::class_<Class, std::shared_ptr<Class>> cls(mod, pyClassName.c_str());

    cls.def(py::init<ast::FrameSet const &, bool>(), "frameSet"_a, "simplify"_a = true);
    cls.def(py::init<ast::Mapping const &, bool>(), "mapping"_a, "simplify"_a = true);

    cls.def("hasForward", &Class::hasForward);
    cls.def("hasInverse", &Class::hasInverse);

    cls.def("getFromEndpoint", &Class::getFromEndpoint);
    // Return a copy of the contained FrameSet in order to assure changing the returned FrameSet
    // will not affect the contained FrameSet (since Python ignores constness)
    cls.def("getFrameSet", [](Class const &self) {
        return self.getFrameSet()->copy();
    });
    cls.def("getToEndpoint", &Class::getToEndpoint);

    cls.def("tranForward", (ToArray (Class::*)(FromArray const &) const) & Class::tranForward, "array"_a);
    cls.def("tranForward", (ToPoint (Class::*)(FromPoint const &) const) & Class::tranForward, "point"_a);
    cls.def("tranInverse", (FromArray (Class::*)(ToArray const &) const) & Class::tranInverse, "array"_a);
    cls.def("tranInverse", (FromPoint (Class::*)(ToPoint const &) const) & Class::tranInverse, "point"_a);
    cls.def("getInverse", &Class::getInverse);
    /* Need some extra handling of ndarray return type in Python to prevent dimensions
     * of length 1 from being deleted */
    cls.def("_getJacobian", &Class::getJacobian);

    declareMethodTemplates<GenericEndpoint, FromEndpoint, ToEndpoint>(cls);
    declareMethodTemplates<Point2Endpoint, FromEndpoint, ToEndpoint>(cls);
    declareMethodTemplates<Point3Endpoint, FromEndpoint, ToEndpoint>(cls);
    declareMethodTemplates<SpherePointEndpoint, FromEndpoint, ToEndpoint>(cls);

    // str(self) = "<Python class name>[<nIn>-><nOut>]"
    cls.def("__str__", [pyClassName](Class const &self) { return formatStr(self, pyClassName); });
    // repr(self) = "lsst.afw.geom.<Python class name>[<nIn>-><nOut>]"
    cls.def("__repr__",
            [pyClassName](Class const &self) { return "lsst.afw.geom." + formatStr(self, pyClassName); });
}

PYBIND11_PLUGIN(transform) {
    py::module mod("transform");

    py::module::import("astshim");
    py::module::import("lsst.afw.geom.endpoint");

    // Need to import numpy for ndarray and eigen conversions
    if (_import_array() < 0) {
        PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
        return nullptr;
    }

    declareTransform<GenericEndpoint, GenericEndpoint>(mod);
    declareTransform<GenericEndpoint, Point2Endpoint>(mod);
    declareTransform<GenericEndpoint, Point3Endpoint>(mod);
    declareTransform<GenericEndpoint, SpherePointEndpoint>(mod);
    declareTransform<Point2Endpoint, GenericEndpoint>(mod);
    declareTransform<Point2Endpoint, Point2Endpoint>(mod);
    declareTransform<Point2Endpoint, Point3Endpoint>(mod);
    declareTransform<Point2Endpoint, SpherePointEndpoint>(mod);
    declareTransform<Point3Endpoint, GenericEndpoint>(mod);
    declareTransform<Point3Endpoint, Point2Endpoint>(mod);
    declareTransform<Point3Endpoint, Point3Endpoint>(mod);
    declareTransform<Point3Endpoint, SpherePointEndpoint>(mod);
    declareTransform<SpherePointEndpoint, GenericEndpoint>(mod);
    declareTransform<SpherePointEndpoint, Point2Endpoint>(mod);
    declareTransform<SpherePointEndpoint, Point3Endpoint>(mod);
    declareTransform<SpherePointEndpoint, SpherePointEndpoint>(mod);

    return mod.ptr();
}

}  // <anonymous>
}  // geom
}  // afw
}  // lsst
