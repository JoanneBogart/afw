#
# LSST Data Management System
# Copyright 2008-2017 LSST/AURA.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.
#

from __future__ import absolute_import, division, print_function

__all__ = ["Mask", "MaskPixel"]

from future.utils import with_metaclass
import numpy as np

from lsst.utils import TemplateMeta
from .mask import MaskX
from ..slicing import supportSlicing

supportSlicing(MaskX)

MaskPixel = np.int32


class Mask(with_metaclass(TemplateMeta, object)):
    TEMPLATE_PARAMS = ("dtype",)
    TEMPLATE_DEFAULTS = (MaskPixel,)
    STATIC_METHODS = ("readFits", "interpret", "parseMaskPlaneMetadata", "clearMaskPlaneDict",
                      "addMaskPlane", "removeMaskPlane", "getMaskPlane", "getPlaneBitMask",
                      "getNumPlanesMax", "getNumPlanesUsed", "addMaskPlanesToMetadata",)

    def __reduce__(self):
        from lsst.afw.fits import reduceToFits
        return reduceToFits(self)


Mask.register(MaskPixel, MaskX)
Mask.alias("X", MaskX)

for cls in (MaskX, ):
    supportSlicing(cls)
