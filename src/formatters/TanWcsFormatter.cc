// -*- lsst-c++ -*-

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

/*
 * Implementation of TanWcsFormatter class
 */

#ifndef __GNUC__
#define __attribute__(x) /*NOTHING*/
#endif
static char const* SVNid __attribute__((unused)) = "$Id$";

#include <string>

//#include "boost/serialization/shared_ptr.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "wcslib/wcs.h"

#include "lsst/daf/base.h"
#include "lsst/daf/persistence.h"
#include "lsst/daf/persistence/PropertySetFormatter.h"
#include "lsst/pex/exceptions.h"
#include "lsst/log/Log.h"
#include "lsst/afw/formatters/ImageFormatter.h"
#include "lsst/afw/formatters/MaskedImageFormatter.h"
#include "lsst/afw/formatters/TanWcsFormatter.h"
#include "lsst/afw/image/TanWcs.h"
#include "lsst/afw/fits.h"

namespace {
LOG_LOGGER _log = LOG_GET("afw.TanWcsFormatter");
}

namespace lsst {
namespace afw {
namespace formatters {

namespace dafBase = lsst::daf::base;
namespace dafPersist = lsst::daf::persistence;
namespace pexPolicy = lsst::pex::policy;
namespace pexExcept = lsst::pex::exceptions;

dafPersist::FormatterRegistration TanWcsFormatter::registration("TanWcs", typeid(image::TanWcs),
                                                                createInstance);

TanWcsFormatter::TanWcsFormatter(std::shared_ptr<pexPolicy::Policy>) : dafPersist::Formatter(typeid(this)) {}

TanWcsFormatter::~TanWcsFormatter() = default;

void TanWcsFormatter::write(dafBase::Persistable const* persistable,
                            std::shared_ptr<dafPersist::FormatterStorage> storage,
                            std::shared_ptr<dafBase::PropertySet>) {
    LOGL_DEBUG(_log, "TamWcsFormatter write start");
    image::TanWcs const* ip = dynamic_cast<image::TanWcs const*>(persistable);
    if (ip == nullptr) {
        throw LSST_EXCEPT(pexExcept::RuntimeError, "Persisting non-TanWcs");
    }
    // TODO: Replace this with something better in DM-10776
    auto boost = std::dynamic_pointer_cast<dafPersist::BoostStorage>(storage);
    if (boost) {
        LOGL_DEBUG(_log, "TanWcsFormatter write BoostStorage");
        boost->getOArchive() & *ip;
        LOGL_DEBUG(_log, "TanWcsFormatter write end");
        return;
    }
    throw LSST_EXCEPT(pexExcept::RuntimeError, "Unrecognized FormatterStorage for TanWcs");
}

dafBase::Persistable* TanWcsFormatter::read(std::shared_ptr<dafPersist::FormatterStorage> storage,
                                            std::shared_ptr<dafBase::PropertySet> additionalData) {
    LOGL_DEBUG(_log, "TanWcsFormatter read start");
    // TODO: Replace this with something better in DM-10776
    auto boost = std::dynamic_pointer_cast<dafPersist::BoostStorage>(storage);
    if (boost) {
        image::TanWcs* ip = new image::TanWcs;
        LOGL_DEBUG(_log, "TanWcsFormatter read BoostStorage");
        boost->getIArchive() & *ip;
        LOGL_DEBUG(_log, "TanWcsFormatter read end");
        return ip;
    }
    auto fits = std::dynamic_pointer_cast<dafPersist::FitsStorage>(storage);
    if (fits) {
        LOGL_DEBUG(_log, "TanWcsFormatter read FitsStorage");
        int hdu = additionalData->get<int>("hdu", INT_MIN);
        std::shared_ptr<dafBase::PropertySet> md = afw::fits::readMetadata(fits->getPath(), hdu);
        image::TanWcs* ip = new image::TanWcs(md);
        LOGL_DEBUG(_log, "TanWcsFormatter read end");
        return ip;
    }
    throw LSST_EXCEPT(pexExcept::RuntimeError, "Unrecognized FormatterStorage for TanWcs");
}

void TanWcsFormatter::update(dafBase::Persistable*, std::shared_ptr<dafPersist::FormatterStorage>,
                             std::shared_ptr<dafBase::PropertySet>) {
    throw LSST_EXCEPT(pexExcept::RuntimeError, "Unexpected call to update for TanWcs");
}

/// @internal Provide a function to serialise an Eigen::Matrix so we can persist the SIP matrices
template <class Archive>
void serializeEigenArray(Archive& ar, Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& m) {
    int rows = m.rows();
    int cols = m.cols();
    ar& rows& cols;
    if (Archive::is_loading::value) {
        m = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>(rows, cols);
    }
    for (int j = 0; j < m.cols(); ++j) {
        for (int i = 0; i < m.rows(); ++i) {
            ar& m(i, j);
        }
    }
}

static void encodeSipHeader(std::shared_ptr<daf::base::PropertySet> wcsProps,
                            std::string const& which,  ///< @internal Either A,B, Ap or Bp
                            Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> const& m) {
    int order = m.rows();
    if (m.cols() != order) {
        throw LSST_EXCEPT(pexExcept::DomainError, "sip" + which + " matrix is not square");
    }
    if (order > 0) {
        order -= 1;  // match SIP convention
        wcsProps->add(which + "_ORDER", static_cast<int>(order));
        for (int i = 0; i <= order; ++i) {
            for (int j = 0; j <= order; ++j) {
                double val = m(i, j);
                if (val != 0.0) {
                    wcsProps->add((boost::format("%1%_%2%_%3%") % which % i % j).str(), val);
                }
            }
        }
    }
}

std::shared_ptr<dafBase::PropertyList> TanWcsFormatter::generatePropertySet(image::TanWcs const& wcs) {
    // Only generates properties for the first wcsInfo.
    std::shared_ptr<dafBase::PropertyList> wcsProps(new dafBase::PropertyList());

    if (wcs._wcsInfo == nullptr) {  // nothing to add
        return wcsProps;
    }

    wcsProps->add("NAXIS", wcs._wcsInfo[0].naxis, "number of data axes");
    // EQUINOX is "not relevant" (FITS definition, version 3.0, page 30) when
    // dealing with ICRS, and may confuse readers. Don't write it.
    if (strncmp(wcs._wcsInfo[0].radesys, "ICRS", 4) != 0) {
        wcsProps->add("EQUINOX", wcs._wcsInfo[0].equinox, "Equinox of coordinates");
    }
    wcsProps->add("RADESYS", std::string(wcs._wcsInfo[0].radesys), "Coordinate system for equinox");
    wcsProps->add("CRPIX1", wcs._wcsInfo[0].crpix[0], "WCS Coordinate reference pixel");
    wcsProps->add("CRPIX2", wcs._wcsInfo[0].crpix[1], "WCS Coordinate reference pixel");
    wcsProps->add("CD1_1", wcs._wcsInfo[0].cd[0], "WCS Coordinate scale matrix");
    wcsProps->add("CD1_2", wcs._wcsInfo[0].cd[1], "WCS Coordinate scale matrix");
    wcsProps->add("CD2_1", wcs._wcsInfo[0].cd[2], "WCS Coordinate scale matrix");
    wcsProps->add("CD2_2", wcs._wcsInfo[0].cd[3], "WCS Coordinate scale matrix");
    wcsProps->add("CRVAL1", wcs._wcsInfo[0].crval[0], "WCS Ref value (RA in decimal degrees)");
    wcsProps->add("CRVAL2", wcs._wcsInfo[0].crval[1], "WCS Ref value (DEC in decimal degrees)");
    wcsProps->add("CUNIT1", std::string(wcs._wcsInfo[0].cunit[0]));
    wcsProps->add("CUNIT2", std::string(wcs._wcsInfo[0].cunit[1]));

    // Hack. Because wcslib4.3 gets confused when it's passed RA---TAN-SIP,
    // we set the value of ctypes to just RA---TAN, regardless of whether
    // the SIP types are present. But when we persist to a file, we need to
    // check whether the SIP polynomials were actually there and correct
    // ctypes if necessary. Bad things will happen if someone tries to
    // use a system other than RA---TAN and DEC--TAN
    std::string ctype1(wcs._wcsInfo[0].ctype[0]);
    std::string ctype2(wcs._wcsInfo[0].ctype[1]);

    if (wcs._hasDistortion) {
        if (ctype1.rfind("-SIP") == std::string::npos) {
            ctype1 += "-SIP";
        }
        if (ctype2.rfind("-SIP") == std::string::npos) {
            ctype2 += "-SIP";
        }
        encodeSipHeader(wcsProps, "A", wcs._sipA);
        encodeSipHeader(wcsProps, "B", wcs._sipB);
        encodeSipHeader(wcsProps, "AP", wcs._sipAp);
        encodeSipHeader(wcsProps, "BP", wcs._sipBp);
    }
    wcsProps->add("CTYPE1", ctype1, "WCS Coordinate type");
    wcsProps->add("CTYPE2", ctype2, "WCS Coordinate type");

    return wcsProps;
}

template <class Archive>
void TanWcsFormatter::delegateSerialize(Archive& ar, int const, dafBase::Persistable* persistable) {
    LOGL_DEBUG(_log, "TanWcsFormatter delegateSerialize start");
    image::TanWcs* ip = dynamic_cast<image::TanWcs*>(persistable);
    if (ip == nullptr) {
        throw LSST_EXCEPT(pexExcept::RuntimeError, "Serializing non-TanWcs");
    }

    // Serialize most fields normally
    ar & ip->_nWcsInfo & ip->_relax;
    ar & ip->_wcsfixCtrl & ip->_wcshdrCtrl & ip->_nReject;
    ar & ip->_coordSystem;

    ar & ip->_hasDistortion;

    if (ip->_hasDistortion) {
        serializeEigenArray(ar, ip->_sipA);
        serializeEigenArray(ar, ip->_sipAp);
        serializeEigenArray(ar, ip->_sipB);
        serializeEigenArray(ar, ip->_sipBp);
    }

    // If we are loading, create the array of Wcs parameter structs
    if (Archive::is_loading::value) {
        ip->_wcsInfo = reinterpret_cast<wcsprm*>(malloc(ip->_nWcsInfo * sizeof(wcsprm)));
    }

    for (int i = 0; i < ip->_nWcsInfo; ++i) {
        // If we are loading, initialize the struct first
        if (Archive::is_loading::value) {
            ip->_wcsInfo[i].flag = -1;
            wcsini(1, 2, &(ip->_wcsInfo[i]));
        }

        // Serialize only critical Wcs parameters
        ar & ip->_wcsInfo[i].naxis;
        ar & ip->_wcsInfo[i].equinox;
        ar & ip->_wcsInfo[i].radesys;
        ar & ip->_wcsInfo[i].crpix[0];
        ar & ip->_wcsInfo[i].crpix[1];
        ar & ip->_wcsInfo[i].cd[0];
        ar & ip->_wcsInfo[i].cd[1];
        ar & ip->_wcsInfo[i].cd[2];
        ar & ip->_wcsInfo[i].cd[3];
        ar & ip->_wcsInfo[i].crval[0];
        ar & ip->_wcsInfo[i].crval[1];
        ar & ip->_wcsInfo[i].cunit[0];
        ar & ip->_wcsInfo[i].cunit[1];
        ar & ip->_wcsInfo[i].ctype[0];
        ar & ip->_wcsInfo[i].ctype[1];
        ar & ip->_wcsInfo[i].altlin;

        // If we are loading, compute intermediate values given those above
        if (Archive::is_loading::value) {
            ip->_wcsInfo[i].flag = 0;
            wcsset(&(ip->_wcsInfo[i]));
        }
    }
    LOGL_DEBUG(_log, "TanWcsFormatter delegateSerialize end");
}

// Explicit template specializations confuse Doxygen, tell it to ignore them
/// @cond
template void TanWcsFormatter::delegateSerialize(boost::archive::text_oarchive&, int, dafBase::Persistable*);
template void TanWcsFormatter::delegateSerialize(boost::archive::text_iarchive&, int, dafBase::Persistable*);
template void TanWcsFormatter::delegateSerialize(boost::archive::binary_oarchive&, int,
                                                 dafBase::Persistable*);
template void TanWcsFormatter::delegateSerialize(boost::archive::binary_iarchive&, int,
                                                 dafBase::Persistable*);
/// @endcond

std::shared_ptr<dafPersist::Formatter> TanWcsFormatter::createInstance(
        std::shared_ptr<pexPolicy::Policy> policy) {
    return std::shared_ptr<dafPersist::Formatter>(new TanWcsFormatter(policy));
}
}
}
}  // end lsst::afw::formatters
