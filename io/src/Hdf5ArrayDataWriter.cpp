/* \file
 * $Id$
 * \author Caton Little
 * \brief 
 *
 * \section LICENSE
 *
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is FieldML
 *
 * The Initial Developer of the Original Code is Auckland Uniservices Ltd,
 * Auckland, New Zealand. Portions created by the Initial Developer are
 * Copyright (C) 2010 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 */

#include "StringUtil.h"
#include "FieldmlIoApi.h"

#include "ArrayDataWriter.h"
#include "Hdf5ArrayDataWriter.h"

using namespace std;

#if defined FIELDML_HDF5_ARRAY || defined FIELDML_PHDF5_ARRAY

Hdf5ArrayDataWriter *Hdf5ArrayDataWriter::create( FieldmlIoContext *context, const string root, FmlObjectHandle source, FieldmlHandleType handleType, bool append, int *sizes, int rank )
{
    Hdf5ArrayDataWriter *writer = NULL;
    
    FmlObjectHandle resource = Fieldml_GetDataSourceResource( context->getSession(), source );
    char *temp_string = Fieldml_GetDataResourceFormat( context->getSession(), resource );
    string format;

    if( !StringUtil::safeString( temp_string, format ) )
    {
        context->setError( FML_IOERR_CORE_ERROR );
    }
    else if( format == StringUtil::HDF5_NAME )
    {
#ifdef FIELDML_HDF5_ARRAY
        Hdf5ArrayDataWriter *hdf5writer = new Hdf5ArrayDataWriter( context, root, source, handleType, append, sizes, rank, H5P_DEFAULT );
        if( !hdf5writer->ok )
        {
            delete hdf5writer;
        }
        else
        {
            writer = hdf5writer;
        }
#endif //FIELDML_HDF5_ARRAY
    }
    else if( format == StringUtil::PHDF5_NAME )
    {
#ifdef FIELDML_PHDF5_ARRAY
        hid_t accessProperties = H5Pcreate( H5P_FILE_ACCESS );
        if( H5Pset_fapl_mpio( accessProperties, MPI_COMM_WORLD, MPI_INFO_NULL ) >= 0 )
        {
            Hdf5ArrayDataWriter *hdf5writer = new Hdf5ArrayDataWriter( context, root, source, handleType, append, sizes, rank, accessProperties );
            if( !hdf5writer->ok )
            {
                delete hdf5writer;
            }
            else
            {
                writer = hdf5writer;
            }
        }
        H5Pclose( accessProperties );
#endif //FIELDML_PHDF5_ARRAY
    }
    Fieldml_FreeString(temp_string);
    
    return writer;
}


Hdf5ArrayDataWriter::Hdf5ArrayDataWriter( FieldmlIoContext *_context, const string root, FmlObjectHandle source, FieldmlHandleType handleType, bool append, int *sizes, int _rank, hid_t accessProperties ) :
    ArrayDataWriter( _context )
{
    rank = _rank;
    file = -1;
    dataset = -1;
    dataspace = -1;
    
    hStrides = NULL;
    hSizes = NULL;
    hOffsets = NULL;
    
    ok = false;
    closed = true;
    
    FmlObjectHandle resource = Fieldml_GetDataSourceResource( context->getSession(), source );
    
    while( true )
    {
        string description;
        char *temp_href = Fieldml_GetDataResourceHref( context->getSession(), resource );
        if( !StringUtil::safeString( Fieldml_GetDataResourceHref( context->getSession(), resource ), description ) )
        {
            break;
        }
        Fieldml_FreeString(temp_href);

        string location;
        char *temp_string = Fieldml_GetArrayDataSourceLocation( context->getSession(), source );
        if( !StringUtil::safeString( temp_string, location ) )
        {
            break;
        }
        Fieldml_FreeString(temp_string);

        const string filename = StringUtil::makeFilename( root, description );
        //TODO Add an API-level enum to allow the user to append data, nuke any existing file, or fail if the file already exists. 
        if( !append )
        {
            file = H5Fcreate( filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, accessProperties );
        }
        else
        {
            file = H5Fopen( filename.c_str(), H5F_ACC_RDWR, accessProperties );
            if( file < 0 )
            {
                file = H5Fcreate( filename.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, accessProperties );
            }
        }
        
        if( file < 0 )
        {
            break;
        }

        hOffsets = new hsize_t[rank];
        hSizes = new hsize_t[rank];
        hStrides = new hsize_t[rank];
        for( int i = 0; i < rank; i++ )
        {
            hStrides[i] = 1;
        }

        //Temporarily disable errors. 
        H5E_auto1_t func;
        void *client_data;
        H5Eget_auto1( &func, &client_data ); 
        H5Eset_auto1( NULL, NULL );
        //It's fine if this call fails.
        dataset = H5Dopen( file, location.c_str(), H5P_DEFAULT );
        H5Eset_auto1( func, client_data );

        if( dataset < 0 )
        {
            if( !initializeWithNewDataset( location, sizes, handleType ) )
            {
                break;
            }
        }
        else
        {
            if( !initializeWithExistingDataset( sizes ) )
            {
                break;
            }
        }

        datatype = H5Dget_type( dataset );
        if( datatype < 0 )
        {
            break;
        }

        hid_t nativeType = H5Tget_native_type( datatype, H5T_DIR_DESCEND );
        if( H5Tequal( nativeType, H5T_NATIVE_INT ) || H5Tequal( nativeType, H5T_NATIVE_SHORT ) )
        {
            datatype = H5T_NATIVE_INT;
        }
        else if( H5Tequal( nativeType, H5T_NATIVE_DOUBLE ) || H5Tequal( nativeType, H5T_NATIVE_FLOAT ) )
        {
            datatype = H5T_NATIVE_DOUBLE;
        }
        else if( H5Tequal( nativeType, H5T_NATIVE_INT8 ) )
        {
            datatype = H5T_NATIVE_INT8;
        }
        
        ok = true;
        closed = false;
        break;
    }
}


bool Hdf5ArrayDataWriter::initializeWithNewDataset( const string location, int *sizes, FieldmlHandleType handleType )
{
    for( int i = 0; i < rank; i++ )
    {
        hSizes[i] = sizes[i];
    }
    dataspace = H5Screate_simple( rank, hSizes, NULL );
    if( dataspace < 0 )
    {
        return false;
    }
    
    if( handleType == FHT_CONTINUOUS_TYPE )
    {
        datatype = H5T_NATIVE_DOUBLE;
    }
    else if( handleType == FHT_ENSEMBLE_TYPE )
    {
        datatype = H5T_NATIVE_INT;
    }
    else if( handleType == FHT_BOOLEAN_TYPE )
    {
        //HDF5 has no boolean type.
        //TODO Bit-packing for booleans.
        datatype = H5T_NATIVE_INT8;
    }
    else
    {
        return false;
    }

    dataset = H5Dcreate( file, location.c_str(), datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
    if( dataset < 0 )
    {
        return false;
    }
    
    return true;
}


bool Hdf5ArrayDataWriter::initializeWithExistingDataset( int *sizes )
{
    //The dataset already exists. Make sure its dataspace is compatible with the one requested.
    dataspace = H5Dget_space( dataset );
    if( dataspace < 0 )
    {
        return false;
    }
    
    int existingRank = H5Sget_simple_extent_ndims( dataspace );
    if( ( existingRank < 0 ) || ( existingRank != rank ) ) 
    {
        return false;
    }
    
    existingRank = H5Sget_simple_extent_dims( dataspace, NULL, hSizes );
    if( existingRank != rank )
    {
        return false;
    }
    
    for( int i = 0; i < rank; i++ )
    {
        if( ( hSizes[i] != H5S_UNLIMITED ) && ( hSizes[i] < sizes[i] ) )
        {
            existingRank = -1;
        }
    }
    if( existingRank < 0 )
    {
        return false;
    }

    //The dataset already exists. Make sure its datatype is compatible with the one requested.
    
    return true;
}


FmlIoErrorNumber Hdf5ArrayDataWriter::writeSlab( const int *offsets, const int *sizes, hid_t requiredDatatype, const void *valueBuffer )
{
    if( datatype != requiredDatatype )
    {
        context->setError( FML_IOERR_UNSUPPORTED );
        return -1;
    }

    for( int i = 0; i < rank; i++ )
    {
        hOffsets[i] = offsets[i];
        hSizes[i] = sizes[i];
    }
    
    hid_t bufferSpace = H5Screate_simple( rank, hSizes, NULL );
    
    herr_t status;
    status = H5Sselect_hyperslab( dataspace, H5S_SELECT_SET, hOffsets, NULL, hSizes, NULL );
    
    hid_t transferProperties = H5P_DEFAULT;

//    if( collective )
//    {
//        transferProperties = H5Pcreate (H5P_DATASET_XFER);
//        H5Pset_dxpl_mpio(transferProperties, H5FD_MPIO_COLLECTIVE);
//    }
    
    status = H5Dwrite( dataset, requiredDatatype, bufferSpace, dataspace, transferProperties, valueBuffer );

//    H5Pclose( transferProperties );    
    
    H5Sclose( bufferSpace );
    
    if( status >= 0 )
    {
        return FML_IOERR_NO_ERROR;
    }
    
    return context->setError( FML_IOERR_WRITE_ERROR );
}


FmlIoErrorNumber Hdf5ArrayDataWriter::writeIntSlab( const int *offsets, const int *sizes, const int *valueBuffer )
{
    if( closed )
    {
        return FML_IOERR_RESOURCE_CLOSED;
    }
    
    return writeSlab( offsets, sizes, H5T_NATIVE_INT, valueBuffer );
}


FmlIoErrorNumber Hdf5ArrayDataWriter::writeDoubleSlab( const int *offsets, const int *sizes, const double *valueBuffer )
{
    if( closed )
    {
        return FML_IOERR_RESOURCE_CLOSED;
    }

    return writeSlab( offsets, sizes, H5T_NATIVE_DOUBLE, valueBuffer );
}


FmlIoErrorNumber Hdf5ArrayDataWriter::writeBooleanSlab( const int *offsets, const int *sizes, const FmlBoolean *valueBuffer )
{
    if( closed )
    {
        return FML_IOERR_RESOURCE_CLOSED;
    }

    return writeSlab( offsets, sizes, H5T_NATIVE_INT8, valueBuffer );
}


FmlIoErrorNumber Hdf5ArrayDataWriter::close()
{
    if( closed )
    {
        return FML_IOERR_NO_ERROR;
    }
    
    closed = true;
    
    H5Sclose( dataspace );
    H5Dclose( dataset );
    H5Fclose( file );
    
    return FML_IOERR_NO_ERROR;
}


Hdf5ArrayDataWriter::~Hdf5ArrayDataWriter()
{
    if( !closed )
    {
        close();
    }
    
    delete[] hStrides;
    delete[] hSizes;
    delete[] hOffsets;
}
#endif //FIELDML_HDF5_ARRAY
