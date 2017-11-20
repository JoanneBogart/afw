/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
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
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#include "pybind11/pybind11.h"

#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/fits.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace table {
namespace io {

using PyPersistable = py::class_<Persistable, std::shared_ptr<Persistable>>;

PYBIND11_PLUGIN(persistable) {
    py::module mod("persistable");
    py::module::import("lsst.afw.fits");

    PyPersistable cls(mod, "Persistable");
    cls.def("writeFits",
            (void (Persistable::*)(std::string const &, std::string const &) const) & Persistable::writeFits,
            "fileName"_a, "mode"_a = "w");
    cls.def("writeFits", (void (Persistable::*)(fits::MemFileManager &, std::string const &) const) &
                                 Persistable::writeFits,
            "manager"_a, "mode"_a = "w");
    cls.def_static("readFits",
                   (std::shared_ptr<Persistable>(*)(std::string const &, int)) & Persistable::readFits,
                   "fileName"_a, "hdu"_a = INT_MIN);
    cls.def_static("readFits",
                   (std::shared_ptr<Persistable>(*)(fits::MemFileManager &, int)) & Persistable::readFits,
                   "manager"_a, "hdu"_a = INT_MIN);
    cls.def("isPersistable", &Persistable::isPersistable);

    return mod.ptr();
}
}
}
}
}  // namespace lsst::afw::table::io