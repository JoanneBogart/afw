// -*- lsst-c++ -*-
#ifndef AFW_TABLE_IO_FitsWriter_h_INCLUDED
#define AFW_TABLE_IO_FitsWriter_h_INCLUDED

#include "boost/shared_ptr.hpp"

#include "lsst/afw/fits.h"
#include "lsst/afw/table/io/Writer.h"

namespace lsst { namespace afw { namespace table { namespace io {

/**
 *  @brief Writer subclass for FITS binary tables.
 *
 *  FitsWriter itself provides support for writing FITS binary tables from base containers.
 *  Derived record/base pairs should derive their own writer from FitsWriter and reimplement
 *  BaseTable::makeFitsWriter to return it.  Subclasses will usually delegate most of the
 *  work back to FitsWriter.
 */
class FitsWriter : public Writer {
public:

    typedef afw::fits::Fits Fits;

    /**
     *  @brief Driver for writing FITS files.
     *
     *  A container class will usually provide a member function that calls this driver,
     *  which opens the FITS file, calls makeFitsWriter on the container's table, and
     *  then calls Writer::write on it.
     */
    template <typename ContainerT>
    static void apply(std::string const & filename, std::string const & mode, ContainerT const & container) {
        Fits fits(filename, mode, Fits::AUTO_CLOSE | Fits::AUTO_CHECK);
        PTR(FitsWriter) writer 
            = boost::static_pointer_cast<BaseTable const>(container.getTable())->makeFitsWriter(&fits);
        writer->write(container);
    }

    /// @brief Construct from a wrapped cfitsio pointer. 
    explicit FitsWriter(Fits * fits) : _fits(fits) {}

protected:

    /// @copydoc Writer::_writeTable
    virtual void _writeTable(CONST_PTR(BaseTable) const & table, std::size_t nRows);

    /// @copydoc Writer::_writeRecord
    virtual void _writeRecord(BaseRecord const & source);

    Fits * _fits;      // wrapped cfitsio pointer
    std::size_t _row;  // which row we're currently processing

private:
    
    struct ProcessRecords;

    boost::shared_ptr<ProcessRecords> _processor; // a private Schema::forEach functor that write records

};

}}}} // namespace lsst::afw::table::io

#endif // !AFW_TABLE_IO_FitsWriter_h_INCLUDED
