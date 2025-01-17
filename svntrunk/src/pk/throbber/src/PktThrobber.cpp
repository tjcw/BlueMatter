/* Copyright 2001, 2019 IBM Corporation
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the 
 * following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the 
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 #ifndef PKFXLOG_PKTTHROBBER_INIT
#define PKFXLOG_PKTTHROBBER_INIT ( 0 )
#endif

#include <pk/platform.hpp>
#include <pk/fxlogger.hpp>

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdlib.h>
#include <rts.h>
#include <BonB/blade_on_blrts.h>

#ifdef PKT_STAND_ALONE
#include <pk/XYZ.hpp>
#include <BonB/BGLTorusAppSupport.c>
#include <pk/a2a/packet_alltoallv.hpp>
#include <pk/platform.hpp>
#include <pk/fxlogger.hpp>

#include <spi/TorusXYZ.hpp>

#include <spi/PktThrobberIF.hpp>

#else
#include <pk/XYZ.hpp>

#include <BonB/BGLTorusAppSupport.c>
#include <pk/a2a/packet_alltoallv.hpp>
//#include <pk/platform.hpp>
//#include <pk/fxlogger.hpp>

#include <spi/TorusXYZ.hpp>

#include <spi/PktThrobberIF.hpp>

#endif

#ifndef THOOD_VERBOSE
#define THOOD_VERBOSE 0
#endif

int Rank = -1;

#ifdef PK_PHASE5_SPLIT_COMMS
  #define THROBBER_THREAD_COUNT (1)  // throbber will run only on core 0
#else
  #define THROBBER_THREAD_COUNT (2)  // throbber will costart other processor to help
#endif

///////TraceClient BCastCoreZeroStart;
///////TraceClient BCastCoreZeroFinis;
///////  BCastCoreZeroStart.HitOE( 1,
///////                     "BCastCoreZeroStart",
///////                     MyAddressSpaceId,
///////                     BCastCoreZeroStart );


#ifndef PKTTH_STD
#define PKTTH_STD   (0) ///(Rank == 0 || Rank == 9)
#endif

#ifndef PKTTH_LOG_HIGH_FREQ
#define PKTTH_LOG_HIGH_FREQ (0)
#endif

#ifndef PKTTH_LOG_GRAPH
#define PKTTH_LOG_GRAPH (PKTTH_STD)
//#define PKTTH_LOG_GRAPH (1)
#endif

#ifndef PKTTH_LOG_ASSIGN
#define PKTTH_LOG_ASSIGN (PKTTH_STD)
#endif

#ifndef PKTTH_LOG_INIT
// #define PKTTH_LOG_INIT (PKTTH_STD)
#define PKTTH_LOG_INIT ( 0)
#endif

#ifndef PKTTH_LOG_BCAST
#define PKTTH_LOG_BCAST (PKTTH_STD)
#endif

#ifndef PKTTH_LOG_REDUCE
#define PKTTH_LOG_REDUCE (PKTTH_STD)
#endif

#ifndef PKTTH_LOG_IF
#define PKTTH_LOG_IF (PKTTH_STD)
#endif

#ifndef COMMGRAPH_VERIFY_SRC
#define COMMGRAPH_VERIFY_SRC (0) 
#endif

#ifndef COMMGRAPH_VERIFY_DEST
#define COMMGRAPH_VERIFY_DEST (0) 
#endif

//#ifndef DETERMINISTIC_REDUCTION
//#define DETERMINISTIC_REDUCTION (1)
//#endif

// quadfloat_integrate adds a (52-bit) 'double' to a (104-bit) 'quad', giving a
// 104-bit result.
// The 'volatile' qualifiers are important, to prevent the optimiser from coalescing 
// floating-point adds and subtracts. They must run as coded to get the extended precision
// right. See, for example, 'Priest 1992', or the 'qd' package in http://crd.lbl.gov/~dhbailey/mpdist/
static void quadfloat_integrate_seq(
		double& new_hi,
		double& new_lo,
		double v0,
		double v1, 
		double v2)
{
	double sum_hi = v0 + v1 ;
	volatile double v_sum_hi = sum_hi ; 
	volatile double chaff_1 = v_sum_hi - v0 ;
	double chaff = chaff_1 - v1 ;
	double sum_lo = v2 - chaff ;
	
	double s_hi = sum_hi + sum_lo ;
	volatile double v_s_hi = s_hi ;
	volatile double s_chaff_1 = v_s_hi - sum_hi ;
	double s_chaff = s_chaff_1 - sum_lo ;
	new_hi = s_hi ;
	new_lo = -s_chaff ;
}
static void quadfloat_integrate(
		double& new_hi,
		double& new_lo,
		double old_hi,
		double old_lo,
		double delta)
{
	double abs_old_hi=fabs(old_hi) ;
	double abs_old_lo=fabs(old_lo) ;
	double abs_delta=fabs(delta) ;
	if( abs_delta > abs_old_hi )
	  {
		quadfloat_integrate_seq(new_hi,new_lo,delta,old_hi,old_lo) ;
	  }
	else if( abs_delta > abs_old_lo)
	  {
		quadfloat_integrate_seq(new_hi,new_lo,old_hi,delta,old_lo) ;
	  }
	else 
	  {
		quadfloat_integrate_seq(new_hi,new_lo,old_hi,old_lo,delta) ;
	  }
		   
}

double
MPI_Wtime()
  {
  unsigned long long now = rts_get_timebase();
  double rc = 1.0 * now;
  rc /= 700 * 1000 * 1000;
  return( rc );
  }

class CommGraph
  {
  double padding0[4];
  public:
  struct CommGraphRecord
    {
    int mRootBoundTaskId;      // TaskId if any, of root bound task
    int mLeafBoundTaskSet[1];  // l-vec ... index 0 is count, then task id's
    };

  struct Edge
    {
    int mFromTaskId;
    int mToTaskId;
    };

  Edge * mEdgeList;
  int mEdgeListCount;
  int mEdgeListSize;

  int                mRootTaskId;
  int                mAddressSpaceCount;
  int                mNodesInNeighborhood;

  char             * mDenseCommGraphHeap;
  CommGraphRecord ** mTaskIdToGraphRecordMap;
  
#if defined(COMMGRAPH_VERIFY_SRC)  
  static int * mRecordSizeRecvData ;
#endif

  double padding1[4];

  CommGraph()
    {
    mRootTaskId          = -1;  // -1 is not set, 0 or more is set.
    mAddressSpaceCount   = -1;
    mNodesInNeighborhood = -1;

    mEdgeList = NULL;
    mEdgeListCount = 0;
    mEdgeListSize  = 0;

    mDenseCommGraphHeap = NULL;
    mTaskIdToGraphRecordMap = NULL;
    }

  void
  Init( int aAddressSpaceCount, int aNodesInNeighborhood )
    {
    mAddressSpaceCount   = aAddressSpaceCount;
    mNodesInNeighborhood = aNodesInNeighborhood;
    }

  void
  SetRoot( int aRootTaskId )
    {
    if( mRootTaskId >= 0 ) PLATFORM_ABORT( "SetRoot(): Called but Root Task Id is already set" );
    mRootTaskId = aRootTaskId;
    }

  ~CommGraph()
    {
    if( mEdgeList != NULL )
      free( mEdgeList );
    if( mDenseCommGraphHeap != NULL )
      free( mDenseCommGraphHeap );
    if( mTaskIdToGraphRecordMap != NULL )
      free( mTaskIdToGraphRecordMap );
    }

  int
  GetRecordLeafLinkCount( int aTaskId )
    {
    assert( aTaskId >= 0 );
    assert( aTaskId < mAddressSpaceCount );
    assert( mTaskIdToGraphRecordMap );
    assert( mDenseCommGraphHeap );
    int rc = mTaskIdToGraphRecordMap[ aTaskId ]->mLeafBoundTaskSet[ 0 ];
    return( rc );
    }

  int
  GetRecordLeafLinkTaskId( int aLeafLinkTaskId, int aLeafNumber )
    {
    assert( aLeafLinkTaskId >= 0 );
    assert( aLeafLinkTaskId < mAddressSpaceCount );
    assert( mTaskIdToGraphRecordMap );
    assert( mDenseCommGraphHeap );
    assert( aLeafNumber >  0 ); // leaf numbers are 1 based
    assert( aLeafNumber <= mTaskIdToGraphRecordMap[ aLeafLinkTaskId ]->mLeafBoundTaskSet[ 0 ] );
    int rc = mTaskIdToGraphRecordMap[ aLeafLinkTaskId ]->mLeafBoundTaskSet[ aLeafNumber ];
    return( rc );
    }

  int
  GetRootBoundLinkTaskId( int aCommPartnerTaskId )
    {
    assert( aCommPartnerTaskId >= 0 );
    assert( aCommPartnerTaskId < mAddressSpaceCount );
    assert( mTaskIdToGraphRecordMap );
    assert( mDenseCommGraphHeap );
    int rc = mTaskIdToGraphRecordMap[ aCommPartnerTaskId ]->mRootBoundTaskId;
    return( rc );
    }

  void
  All2All_Exchange()
    {
    // First, pull out all the record leaf counts so we can exchange the sixe of the recs
    int *  RecordSizeSendAddrVec = (int *) malloc( mAddressSpaceCount * sizeof(int) + CACHE_ISOLATION_PADDING_SIZE );
    if( !  RecordSizeSendAddrVec ) PLATFORM_ABORT("All2All_Exchange(): Failed to alloc RecordSizeSendAddrVec");
    bzero( RecordSizeSendAddrVec, mAddressSpaceCount * sizeof(int) );

    int *  RecordSizeRecvAddrVec = (int *) malloc( mAddressSpaceCount * sizeof(int) + CACHE_ISOLATION_PADDING_SIZE );
    if( !  RecordSizeRecvAddrVec ) PLATFORM_ABORT("All2All_Exchange(): Failed to alloc RecordSizeRecvAddrVec");
    bzero( RecordSizeRecvAddrVec, mAddressSpaceCount * sizeof(int) );

    // First, pull out all the record leaf counts so we can exchange the sixe of the recs
    int *  RecordSizeSendSizeVec = (int *) malloc( mAddressSpaceCount * sizeof(int) + CACHE_ISOLATION_PADDING_SIZE );
    if( !  RecordSizeSendSizeVec ) PLATFORM_ABORT("All2All_Exchange(): Failed to alloc RecordSizeSendSizeVec");
    bzero( RecordSizeSendSizeVec, mAddressSpaceCount * sizeof(int) );

    int *  RecordSizeRecvSizeVec = (int *) malloc( mAddressSpaceCount * sizeof(int) + CACHE_ISOLATION_PADDING_SIZE );
    if( !  RecordSizeRecvSizeVec ) PLATFORM_ABORT("All2All_Exchange(): Failed to alloc RecordSizeRecvSizeVec");
    bzero( RecordSizeRecvSizeVec, mAddressSpaceCount * sizeof(int) );

    // First, pull out all the record leaf counts so we can exchange the sixe of the recs
    int *  RecordSizeSendData = (int *) malloc( mAddressSpaceCount * sizeof(int) + CACHE_ISOLATION_PADDING_SIZE );
    if( !  RecordSizeSendData ) PLATFORM_ABORT("All2All_Exchange(): Failed to alloc RecordSizeSendData");
    bzero( RecordSizeSendData, mAddressSpaceCount * sizeof(int) );

    int *  RecordSizeRecvData = (int *) malloc( mAddressSpaceCount * sizeof(int) + CACHE_ISOLATION_PADDING_SIZE );
    if( !  RecordSizeRecvData ) PLATFORM_ABORT("All2All_Exchange(): Failed to alloc RecordSizeRecvData");
    bzero( RecordSizeRecvData, mAddressSpaceCount * sizeof(int) );

    for(int i = 0; i < mAddressSpaceCount; i++ )
      {
      if( mTaskIdToGraphRecordMap[ i ] )
        {
        RecordSizeSendData[ i ] = sizeof( CommGraphRecord )
                                + (sizeof(int) * mTaskIdToGraphRecordMap[ i ]->mLeafBoundTaskSet[ 0 ]);
        }
      else
        {
        RecordSizeSendData[ i ] = 0;
        }
      RecordSizeSendSizeVec [ i ] = sizeof( int );
      RecordSizeSendAddrVec [ i ] = (int) &(RecordSizeSendData[ i ]);

      RecordSizeRecvData    [ i ] = 0;
      RecordSizeRecvSizeVec [ i ] = sizeof( int );
      RecordSizeRecvAddrVec [ i ] = (int) &(RecordSizeRecvData[ i ]);
      BegLogLine(PKTTH_LOG_INIT)
        << "All2All_Exchange(): RecordSizeSendData[" << i
        << "]=" << RecordSizeSendData[ i ]
        << EndLogLine ;
      }


    static PacketAlltoAllv InitAlltoallv;

    InitAlltoallv.ExecuteSingleCoreSimple( NULL,
                                           RecordSizeSendSizeVec,
                                           RecordSizeSendAddrVec,
                                           sizeof( char ),
                                           NULL,
                                           RecordSizeRecvSizeVec,
                                           RecordSizeRecvAddrVec,
                                           sizeof( char ),
                                           Platform::Topology::GetGroup(), 0 );


    // Allocate a new TaskIdToGraphRecordMap for the exchanged graphs
    CommGraphRecord ** NewTaskIdToGraphRecordMap = (CommGraphRecord **) malloc( mAddressSpaceCount * sizeof( CommGraphRecord * ) + CACHE_ISOLATION_PADDING_SIZE );
    if( NewTaskIdToGraphRecordMap == NULL ) PLATFORM_ABORT("ProcessEdges(): Could not allocated NewTaskIdToGraphRecordMap");
    bzero( NewTaskIdToGraphRecordMap, mAddressSpaceCount * sizeof( CommGraphRecord * ) );

    int NewDenseCommGraphHeapSize = 0;
    for(int i = 0; i < mAddressSpaceCount; i++ )
      {
      NewDenseCommGraphHeapSize += RecordSizeRecvData[ i ];
      }

    char * NewDenseCommGraphHeap = (char *) malloc( NewDenseCommGraphHeapSize + CACHE_ISOLATION_PADDING_SIZE);
    if( !  NewDenseCommGraphHeap ) PLATFORM_ABORT("All2All_Exchange(): Failed to alloc NewDenseCommGraphHeap");
    bzero( NewDenseCommGraphHeap, NewDenseCommGraphHeapSize );

    int    NewNodesInNeighborhood = 0;
    char * ExCGBufPtr = NewDenseCommGraphHeap;
    for(int i = 0; i < mAddressSpaceCount; i++ )
      {
        BegLogLine(PKTTH_LOG_INIT)
          << "All2All_Exchange(): RecordSizeRecvData[" << i
          << "]=" << RecordSizeRecvData[ i ]
          << EndLogLine;
      if( RecordSizeRecvData[ i ] )
        {
        BegLogLine(PKTTH_LOG_INIT)
          << "All2All_Exchange():"
          << " " << NewNodesInNeighborhood
          << " RootTaskId " << i
          << EndLogLine;
        NewTaskIdToGraphRecordMap [ i ] = (CommGraphRecord *) ExCGBufPtr;
        ExCGBufPtr                     += RecordSizeRecvData[ i ];
        NewNodesInNeighborhood ++;
        }
      }

    InitAlltoallv.ExecuteSingleCoreSimple( NULL,
                                           RecordSizeSendData,        // sizes of the local graph nodes as send
                                           (int *) mTaskIdToGraphRecordMap,   // addr of local graph nodes in mapp
                                           sizeof( char ),
                                           NULL,
                                           RecordSizeRecvData,        // sizes of graph nodes to be recved
                                           (int *) NewTaskIdToGraphRecordMap, // addr in new heap
                                           sizeof( char ),
                                           Platform::Topology::GetGroup(), 0 );
    
    // Swap in the new exchaned map and delete the old and temps
    free( mDenseCommGraphHeap );
    mDenseCommGraphHeap = NewDenseCommGraphHeap;
    free( mTaskIdToGraphRecordMap );
    mTaskIdToGraphRecordMap = NewTaskIdToGraphRecordMap;

    if( mNodesInNeighborhood != NewNodesInNeighborhood )
      {
      BegLogLine(PKTTH_LOG_INIT)
        << "All2All_Exchange(): Asymetric bcast neighborhoods "
        << " Old NodesInNeighborhood " << mNodesInNeighborhood
        << " New NodesInNeighborhood " << NewNodesInNeighborhood
        << EndLogLine;

      mNodesInNeighborhood = NewNodesInNeighborhood;
      }

    // Free the temporary areas
    free( RecordSizeSendData    );
    free( RecordSizeSendSizeVec );
    free( RecordSizeSendAddrVec );
    free( RecordSizeRecvSizeVec );
    free( RecordSizeRecvAddrVec );
#if defined(COMMGRAPH_VERIFY_SRC)  
    mRecordSizeRecvData = RecordSizeRecvData ;
#else
    free( RecordSizeRecvData    );
#endif    

    }

  void
  AddEdge( int aToTaskId, int aFromTaskId )
    {
    if( mEdgeListCount == mEdgeListSize )
      {
      mEdgeListSize += 1024;
      Edge * NewEdgeList = (Edge *) malloc( mEdgeListSize * sizeof( Edge ) + CACHE_ISOLATION_PADDING_SIZE);
      if( NewEdgeList == NULL ) PLATFORM_ABORT( "AddEdge(): Failed to allocate EdgeList buffer" );
      if( mEdgeList == NULL )
        {
        memcpy( NewEdgeList, mEdgeList, (mEdgeListCount * sizeof( Edge ) ) );
        free( mEdgeList );
        }
      mEdgeList = NewEdgeList;
      }
    mEdgeList[ mEdgeListCount ].mFromTaskId = aFromTaskId;
    mEdgeList[ mEdgeListCount ].mToTaskId = aToTaskId;
    mEdgeListCount++ ;
    }

  static
  int
  CompareEdge (const void * a, const void * b)
    {
    Edge * EdgeA = (Edge*) a;
    Edge * EdgeB = (Edge*) b;
    if( EdgeA->mFromTaskId != EdgeB->mFromTaskId )
      return( EdgeA->mFromTaskId - EdgeB->mFromTaskId );

    return( EdgeA->mToTaskId - EdgeB->mToTaskId );
    }

  void
  ProcessEdges( int aNodesInHoodCount )
    {
    assert( mAddressSpaceCount > 0 );
    assert( aNodesInHoodCount > 0 );  // i guess this could be zero ... but would need testing.

    // Sort edge set grouping by edges departing a given node to prepare to form into records.
    Platform::Algorithm::Qsort( mEdgeList, mEdgeListCount, sizeof( Edge ), CompareEdge );

    for( int EdgeIndex = 0; EdgeIndex < mEdgeListCount; EdgeIndex++ )
      {
      BegLogLine( PKTTH_LOG_GRAPH)
        << "ProcessEdges(): Dump Sorted Edges "
        << " Edge # " << EdgeIndex
        << " FromTaskId " << mEdgeList[ EdgeIndex ].mFromTaskId
        << " ToTaskId "   << mEdgeList[ EdgeIndex ].mToTaskId
        << EndLogLine;
      }

    // Calculate the memory footprint required for the dense form
    // This form will have a record per graph node
    int RecordFormFootPrint = mEdgeListCount    * sizeof( int ) // one leaf bound entry for each edge
                            + aNodesInHoodCount * sizeof( CommGraphRecord ); // headers for record for each nod in hood

    // Allocate memory for the dense form of the comm graph.
    mDenseCommGraphHeap = (char *) malloc( RecordFormFootPrint + CACHE_ISOLATION_PADDING_SIZE);
    if( mDenseCommGraphHeap == NULL ) PLATFORM_ABORT("ProcessEdges(): Could not allocated mDenseCommGraphHeap");
    bzero( mDenseCommGraphHeap, RecordFormFootPrint );

    mTaskIdToGraphRecordMap = (CommGraphRecord **) malloc( mAddressSpaceCount * sizeof( CommGraphRecord * ) + CACHE_ISOLATION_PADDING_SIZE);
    if( mTaskIdToGraphRecordMap == NULL ) PLATFORM_ABORT("ProcessEdges(): Could not allocated mTaskIdToGraphRecordMap");
    bzero( mTaskIdToGraphRecordMap, mAddressSpaceCount * sizeof( CommGraphRecord * ) );

    int HeapOffset = 0;

    // This scan fills in the entry for all nodes that send to leave bound nodes
    int               CurrentRecordTaskId = -1;
    CommGraphRecord * CurrentRecordPtr    = NULL;
    for( int EdgeIndex = 0; EdgeIndex < mEdgeListCount; EdgeIndex++ )
      {
      if( CurrentRecordTaskId != mEdgeList[ EdgeIndex ].mFromTaskId )
        {
        CurrentRecordTaskId = mEdgeList[ EdgeIndex ].mFromTaskId;
        CurrentRecordPtr    = (CommGraphRecord *) &(mDenseCommGraphHeap[ HeapOffset ]);
        BegLogLine(PKTTH_LOG_GRAPH)
          << "ProcessEdges(): New Rec"
          << " EdgeIndex " << EdgeIndex
          << " FromTaskId " << mEdgeList[ EdgeIndex ].mFromTaskId
          << " ToTaskId "   << mEdgeList[ EdgeIndex ].mToTaskId
          << " Offset " << HeapOffset
          << " RecordPtr " << (void*) CurrentRecordPtr
          << EndLogLine;
        HeapOffset         += sizeof( CommGraphRecord );
        if( HeapOffset > RecordFormFootPrint ) PLATFORM_ABORT("ProcessEdges(): overran alloc");
        CurrentRecordPtr->mRootBoundTaskId       = -1;  // need a second scan
        CurrentRecordPtr->mLeafBoundTaskSet[ 0 ] = 0;
        mTaskIdToGraphRecordMap[ CurrentRecordTaskId ] = CurrentRecordPtr;
        }
      // Record the ToTaskIndex into the record
      CurrentRecordPtr->mLeafBoundTaskSet[ 0 ]++ ;
      CurrentRecordPtr->mLeafBoundTaskSet[ CurrentRecordPtr->mLeafBoundTaskSet[ 0 ] ] =
          mEdgeList[ EdgeIndex ].mToTaskId;

        BegLogLine(PKTTH_LOG_GRAPH)
          << "ProcessEdges(): Added to Rec"
          << " EdgeIndex " << EdgeIndex
          << " FromTaskId " << mEdgeList[ EdgeIndex ].mFromTaskId
          << " ToTaskId "   << mEdgeList[ EdgeIndex ].mToTaskId
          << " Offset " << HeapOffset
          << " RecordPtr " << (void*) CurrentRecordPtr
          << " added link " <<  CurrentRecordPtr->mLeafBoundTaskSet[ 0 ]
          << " task id " << CurrentRecordPtr->mLeafBoundTaskSet[ CurrentRecordPtr->mLeafBoundTaskSet[ 0 ] ]
          << EndLogLine;

      HeapOffset         += sizeof( int );
      }

    // Rescan to add the nodes that are true leaves - the ones that have no leaf bound links
    for( int EdgeIndex = 0; EdgeIndex < mEdgeListCount; EdgeIndex++ )
      {
      int Edge_ToTaskId = mEdgeList[ EdgeIndex ].mToTaskId;

      // If this ToTaskId isn't in the map, it is a leaf node - so add to graph.
      if( !  mTaskIdToGraphRecordMap[ Edge_ToTaskId ] )
        {
        // Add graph entry for the leave node
        CurrentRecordPtr    = (CommGraphRecord *) &(mDenseCommGraphHeap[ HeapOffset ]);
        HeapOffset         += sizeof( CommGraphRecord );
        if( HeapOffset > RecordFormFootPrint ) PLATFORM_ABORT("ProcessEdges(): overran alloc");
        CurrentRecordPtr->mRootBoundTaskId       = -1;  // need a second scan
        CurrentRecordPtr->mLeafBoundTaskSet[ 0 ] = 0;
        mTaskIdToGraphRecordMap[ Edge_ToTaskId ] = CurrentRecordPtr;
        }
      }

    // Now rescan the edge list to fill in the root bound task id
    // Rescan to add the nodes that are true leaves - the ones that have no leaf bound links
    for( int EdgeIndex = 0; EdgeIndex < mEdgeListCount; EdgeIndex++ )
      {
      int Edge_FromTaskId  = mEdgeList[ EdgeIndex ].mFromTaskId;
      int Edge_ToTaskId    = mEdgeList[ EdgeIndex ].mToTaskId;

      CommGraphRecord * FromCurrentRecordPtr = mTaskIdToGraphRecordMap[ Edge_FromTaskId ];
      CommGraphRecord * ToCurrentRecordPtr   = mTaskIdToGraphRecordMap[ Edge_ToTaskId ];

      if(    FromCurrentRecordPtr == NULL
          || ToCurrentRecordPtr   == NULL )
        {
        BegLogLine(PKTTH_LOG_GRAPH)
          << "ProcessEdges(): "
          << " Bad map entry for Edge # " << EdgeIndex
          << " FromTaskId " << Edge_FromTaskId << " Map ptr @ " << (void *) FromCurrentRecordPtr
          << " ToTaskId " << Edge_ToTaskId << " Map ptr @ " << (void *) ToCurrentRecordPtr
          << EndLogLine;
        if( ! FromCurrentRecordPtr ) PLATFORM_ABORT("ProcessEdges(): Bad map value for FromTaskId");
        if( ! ToCurrentRecordPtr ) PLATFORM_ABORT("ProcessEdges(): Bad map value for ToTaskId");
        }

      // While we are here, as a double check, see that all LeafBoundLinks are hooked up right.
      int Found = 0;
      for( int i = 1; i <= FromCurrentRecordPtr->mLeafBoundTaskSet[ 0 ]; i++ )
        {
        if( FromCurrentRecordPtr->mLeafBoundTaskSet[ i ] == Edge_ToTaskId )
          {
          Found = 1;
          break;
          }
        }
      if( FromCurrentRecordPtr->mLeafBoundTaskSet[ 0 ] != 0 && ! Found )
        {
        BegLogLine(PKTTH_LOG_GRAPH)
          << "ProcessEdges(): "
          << " Expected LeafBoundLink not found: Edge # " << EdgeIndex
          << " FromTaskId " << Edge_FromTaskId
          << " ToTaskId " << Edge_ToTaskId
          << EndLogLine;
        PLATFORM_ABORT("ProcessEdges(): graph node LeafBoundLink missing");
        }

      // This edge leads to a node that is already in the graph
      // First check that the RootBoundTaskId isn't previously set
      if( ToCurrentRecordPtr->mRootBoundTaskId != -1
          &&
          ToCurrentRecordPtr->mRootBoundTaskId != Edge_FromTaskId
        )
        {
        BegLogLine(PKTTH_LOG_GRAPH)
          << "ProcessEdges(): "
          << " RootBoundTask with bad value"
          << " EdgeIndex "         << EdgeIndex
          << " Edge_ToTaskId "     << Edge_ToTaskId
          << " RootBoundTask cur " << mTaskIdToGraphRecordMap[ Edge_ToTaskId ]->mRootBoundTaskId
          << " will set too "      << mEdgeList[ EdgeIndex ].mFromTaskId
          << EndLogLine;
        PLATFORM_ABORT("ProcessEdges(): mRootBoundTaskId not unset and not set correctly");
        }

      // Fill in the graph node's RootBoundTaskId -- maybe be written with same data more than once
      ToCurrentRecordPtr->mRootBoundTaskId = Edge_FromTaskId;
      }

    free( mEdgeList );
    mEdgeList = NULL;
    mEdgeListSize = 0;
    mEdgeListCount = 0;

    BegLogLine(PKTTH_LOG_GRAPH)
      << "ProcessEdges(): "
      << "Done."
      << EndLogLine;
    return;
    }

  // This dumps the dense comm graph either before or after the A2A exchange.
  void
  DumpDenseCommGraph( char * aContextStr )
    {
    for(int i = 0; i < mAddressSpaceCount; i++ )
      {
      CommGraphRecord *NodeRecordPtr = mTaskIdToGraphRecordMap[ i ];
      if( NodeRecordPtr != NULL )
        {
        char StrBuf[1024];
        StrBuf[0] = '\0';
        int NextOffset = 0;
        for(int j = 0; j < NodeRecordPtr->mLeafBoundTaskSet[0]; j++ )
          {
          int rc = sprintf( &(StrBuf[ NextOffset ]), " % 5d", NodeRecordPtr->mLeafBoundTaskSet[ j + 1 ] );
          if( rc < 0 ) PLATFORM_ABORT("DumpDenseCommGraph(): sprintf failed");
          NextOffset += rc;
          }
        BegLogLine(PKTTH_LOG_GRAPH)
          << "DumpDenseCommGraph() "   << aContextStr
          << " This Task "             << Rank
          << " Node Map Index "        << i
          << " RootBoundLink "         << NodeRecordPtr->mRootBoundTaskId
          << " LeafBoundSet Count "    << NodeRecordPtr->mLeafBoundTaskSet[ 0 ]
          << " Links "                 << StrBuf
          << EndLogLine;
        }
      }
    }

//  // This dumps the dense comm graph either before or after the A2A exchange.
//  void
//  ConvertTaskIdToXYZInDenseCommGraph()
//    {
//    for(int i = 0; i < mAddressSpaceCount; i++ )
//      {
//      CommGraphRecord *NodeRecordPtr = mTaskIdToGraphRecordMap[ i ];
//      if( NodeRecordPtr != NULL )
//        {
//        for(int j = 0; j < NodeRecordPtr->mLeafBoundTaskSet[0]; j++ )
//          {
//          NodeRecordPtr->mLeafBoundTaskSet[ j + 1 ] ;
//          }
//        }
//      }
//    }

  };

#if defined(COMMGRAPH_VERIFY_SRC)  
  int * CommGraph::mRecordSizeRecvData ;
#endif

  TorusXYZ MakeTaskXYZ( int );

  int
  MakeTaskId( TorusXYZ aLoc )
    {
    int rc = Platform::Topology::MakeRankFromCoords( aLoc.mX, aLoc.mY, aLoc.mZ );

    #ifndef NDEBUG
      if( aLoc != MakeTaskXYZ( rc ) ) PLATFORM_ABORT( "MakeTaskId() - doesn't match MakeTaskXYZ");
    #endif

    return( rc );
    }

  TorusXYZ
  MakeTaskXYZ( int aTaskId )
    {
    int x, y, z;
    Platform::Topology::GetCoordsFromRank( aTaskId, &x, &y, &z );
    TorusXYZ rc;
    rc.Set( x, y, z );
    #ifndef NDEBUG
      if( aTaskId != Platform::Topology::MakeRankFromCoords( rc.mX, rc.mY, rc.mZ) )
          PLATFORM_ABORT("MakeTaskXYZ() - doesn't match MakeTaskId");
    #endif
    return( rc );
    }

  static
  int
  CompareI (const void * a, const void * b)
    {
    int ca = *((int *) a );
    int cb = *((int *) b );
    return( ca - cb );
    }

  void
  CreateCommmGraph_MostDirectRoute( int         aPX, int aPY, int aPZ,
                                    int         aRootTaskId,
                                    int*        aNodesInHoodList,
                                    int         aNodesInHoodCount,
                                    CommGraph & CG )
    {
    int RootTaskIdFound = aNodesInHoodList[ 0 ] == aRootTaskId;
    // The neighborhood list must be sorted for bsearch to work
    for( int i = 1; i < aNodesInHoodCount; i++ )
      {
      if( aNodesInHoodList[ i - 1 ] >= aNodesInHoodList[ i ] )
        {
        BegLogLine(PKTTH_LOG_GRAPH)
          << "CreateCommmGraph_MostDirectRoute(): Out of order "
          << aNodesInHoodList[ i - 1 ]
          << " "
          << aNodesInHoodList[ i ]
          << EndLogLine;
        PLATFORM_ABORT(" CreateCommmGraph_MostDirectRoute(): Neighborhood list was not sorted. ");
        }
      if( aNodesInHoodList[ i ] == aRootTaskId )
         RootTaskIdFound++;
      }
    if( RootTaskIdFound != 1 )
      PLATFORM_ABORT(" CreateCommmGraph_MostDirectRoute(): RootTaskId not in Neighborhood list once and only once. ");

    TorusXYZ PartitionXYZ;
    PartitionXYZ.Set( aPX, aPY, aPZ );
    TorusXYZ RootTaskXYZ = MakeTaskXYZ( aRootTaskId );

    if(THOOD_VERBOSE) printf("% 4d %12.8f :: Nieghborhood Count %d RootTaskId %d RootTaskXYZ % 4d % 4d % 4d PartitionXYZ %d %d %d\n",
       Rank, MPI_Wtime(), aNodesInHoodCount, aRootTaskId, RootTaskXYZ.mX, RootTaskXYZ.mY, RootTaskXYZ.mZ, PartitionXYZ.mX, PartitionXYZ.mY, PartitionXYZ.mZ );

    // Set up the trueus link graph using the 'most direct route' alg = record only root bound links
    // Data form in Graph is changed after recording the root bound links to one more directly useful.
    for(int i = 0; i < aNodesInHoodCount; i++ )
      {
      int      FocusTaskId  = aNodesInHoodList[ i ] ;
      TorusXYZ FocusTaskXYZ = MakeTaskXYZ( FocusTaskId );

      // return an XYZ with a +1 or -1 in one coord indicating the shortest path to the Root tast.
      TorusXYZ RootBoundLink = GetRootBoundTreeLink( PartitionXYZ, RootTaskXYZ, FocusTaskXYZ );

      if( RootBoundLink == TorusXYZ::ZERO_VALUE() )
        {
        if(THOOD_VERBOSE) printf("% 4d %12.8f :: FocusTaskId %d RootTaskId %d\n", Rank, MPI_Wtime(), FocusTaskId, aRootTaskId );
        assert( FocusTaskId == aRootTaskId );
        CG.SetRoot( FocusTaskId );
        }
      else
        {
        // Pick a root bound direction and loop until linked to a node in the group (maybe the root itself).
        TorusXYZ SendToXYZ = FocusTaskXYZ + RootBoundLink;
        SendToXYZ          = SendToXYZ.GetImageInPositiveOctant( PartitionXYZ );
        int SendToTaskId   = MakeTaskId( SendToXYZ );

        // While the SendToTask isn't in the Niegborhood list, keep stepping toward the root.
        while( ! bsearch( (void *)(& SendToTaskId    ),
                          (void *)   aNodesInHoodList ,
                                     aNodesInHoodCount,
                                     sizeof( int     ),
                                     CompareI         ) )
          {
          TorusXYZ RootBoundLink_Of_SendToXYZ = GetRootBoundTreeLink( PartitionXYZ, RootTaskXYZ, SendToXYZ );
          SendToXYZ   += RootBoundLink_Of_SendToXYZ;
          SendToXYZ    = SendToXYZ.GetImageInPositiveOctant( PartitionXYZ );
          SendToTaskId = MakeTaskId( SendToXYZ );

          if(THOOD_VERBOSE) printf("% 4d %12.8f :: Neighbor on rootbound link not in group - trying [ % 4d, % 4d, % 4d ]   \n",
                 Rank, MPI_Wtime(), SendToXYZ );
          }

        // Add an edge to the graph.
        CG.AddEdge( FocusTaskId, SendToTaskId );

        if(THOOD_VERBOSE)
            printf("% 4d %12.8f :: Treus Node % 4d [ %d %d %d ]  Root Links To % 4d [ %d %d %d ]\n",
              Rank, MPI_Wtime(),
              FocusTaskId,  FocusTaskXYZ.mX, FocusTaskXYZ.mY, FocusTaskXYZ.mZ,
              SendToTaskId,  SendToXYZ.mX, SendToXYZ.mY, SendToXYZ.mZ );
        }
      }
    }

  void
  CreateCommmGraph_SkinTree( int         aPX, int aPY, int aPZ,
                                    int         aRootTaskId,
                                    int*        aNodesInHoodList,
                                    int         aNodesInHoodCount,
                                    CommGraph & CG )
    {
    int RootTaskIdFound = aNodesInHoodList[ 0 ] == aRootTaskId;
    // The neighborhood list must be sorted for bsearch to work
    for( int i = 1; i < aNodesInHoodCount; i++ )
      {
      if( aNodesInHoodList[ i - 1 ] >= aNodesInHoodList[ i ] )
        {
        BegLogLine(PKTTH_LOG_GRAPH)
          << "CreateCommmGraph_SkinTree(): Out of order "
          << aNodesInHoodList[ i - 1 ]
          << " "
          << aNodesInHoodList[ i ]
          << EndLogLine;
        PLATFORM_ABORT(" CreateCommmGraph_SkinTree(): Neighborhood list was not sorted. ");
        }
      if( aNodesInHoodList[ i ] == aRootTaskId )
         RootTaskIdFound++;
      }
    if( RootTaskIdFound != 1 )
      PLATFORM_ABORT(" CreateCommmGraph_SkinTree(): RootTaskId not in Neighborhood list once and only once. ");

    TorusXYZ PartitionXYZ;
    PartitionXYZ.Set( aPX, aPY, aPZ );
    TorusXYZ RootTaskXYZ = MakeTaskXYZ( aRootTaskId );

    if(THOOD_VERBOSE) printf("% 4d %12.8f :: Nieghborhood Count %d RootTaskId %d RootTaskXYZ % 4d % 4d % 4d PartitionXYZ %d %d %d\n",
       Rank, MPI_Wtime(), aNodesInHoodCount, aRootTaskId, RootTaskXYZ.mX, RootTaskXYZ.mY, RootTaskXYZ.mZ, PartitionXYZ.mX, PartitionXYZ.mY, PartitionXYZ.mZ );

#define NUM_GATHERERS (6)

    int      Gatherer    [NUM_GATHERERS];
    TorusXYZ GathererXYZ [NUM_GATHERERS];
    int      GathererDist[NUM_GATHERERS];

    GathererDist[0] = 99999999;

    // Find the first gatherer node.
    for(int i = 0; i < aNodesInHoodCount; i++ )
      {
      if( aNodesInHoodList[ i ] == aRootTaskId )
        continue;
      int Dist = RootTaskXYZ.ManhattanDistanceImaged( PartitionXYZ, MakeTaskXYZ( aNodesInHoodList[ i ] ) );
      if( Dist < GathererDist[ 0 ] )
        {
        Gatherer[0]     = aNodesInHoodList[ i ];
        GathererDist[0] = Dist;
        GathererXYZ[0]  = MakeTaskXYZ( aNodesInHoodList[ i ] );
        }
      }

    // pick the other gatherers
    for( int g = 1; g < NUM_GATHERERS; g++ )
      {
      GathererDist[ g ] = 0;

      for(int i = 0; i < aNodesInHoodCount; i++ )
        {
        // don't consider the root as a the next gatherer
        if( aNodesInHoodList[ i ] == aRootTaskId )
          continue;
        int skip = 0;
        // don't consider a current gatherer as the next gatherer
        for(int j = 0; j < g; j++ )
          {
          if( aNodesInHoodList[ i ] == Gatherer[ j ] )
            {
            skip = 1;
            break;
            }
          }
        if( skip )
          continue;

        TorusXYZ iXYZ = MakeTaskXYZ( aNodesInHoodList[ i ] );

        // find the minimum distance to any of the gathers for the considered node
        int Dist = 999999999;
        for(int j = 0; j < g-1; j++ )
          {
          int d = GathererXYZ[j].ManhattanDistanceImaged( PartitionXYZ, iXYZ );
          if( d < Dist )
            Dist = d;
          }
        // if this minimum distance is greater than
        if( Dist > GathererDist[g] )
          {
          GathererDist[g] = Dist;
          Gatherer[g]     = aNodesInHoodList[ i ];
          GathererXYZ[g]  = iXYZ;
          }
        }
      }

    CG.SetRoot( aRootTaskId );


    for( int g = 0; g < NUM_GATHERERS; g++ )
      {
      BegLogLine(PKTTH_LOG_GRAPH)
        << "Gatherer " << g
        << " TaskId "  << Gatherer[g]
        << " XYZ "     << GathererXYZ[g]
        << EndLogLine;
      // Add an edge to the graph.
      CG.AddEdge( Gatherer[g], aRootTaskId );
      }

    int * SelectGatherer = (int *) malloc( aNodesInHoodCount * sizeof( int ) + CACHE_ISOLATION_PADDING_SIZE);
    if( SelectGatherer == NULL ) PLATFORM_ABORT(" Throbber ran out of memory");
    for( int s = 0; s < aNodesInHoodCount; s++ )
      SelectGatherer[ s ] = -1;

    // assign each node to the nearest gatherer
    for(int i = 0; i < aNodesInHoodCount; i++ )
      {
      if( aNodesInHoodList[ i ] == aRootTaskId )
        continue;

      TorusXYZ iXYZ = MakeTaskXYZ( aNodesInHoodList[ i ] );

      SelectGatherer[ i ] = 0;
      int      SelectGathererDist  =
                     GathererXYZ[ 0 ].ManhattanDistanceImaged( PartitionXYZ, iXYZ );
      for(int g = 1; g < NUM_GATHERERS; g++ )
        {
        int d = GathererXYZ[ g ].ManhattanDistanceImaged( PartitionXYZ, iXYZ );
        if( d < SelectGathererDist )
          {
          SelectGatherer[ i ] = g;
          SelectGathererDist  = d;
          }
        }

      }

    for(int i = 0; i < aNodesInHoodCount; i++ )
      {
      BegLogLine(PKTTH_LOG_GRAPH)
        << " i "                      << i
        << " aNodesInHoodList[ i ]  " << aNodesInHoodList[ i ]
        << " SelectGatherer[ i ]"     << SelectGatherer[ i ]
        << " Gatherer[ SelectGatherer [ i ] ] " << Gatherer[ SelectGatherer[ i ] ]
        << EndLogLine;
      }


    for(int i = 0; i < aNodesInHoodCount; i++ )
      {
      if( aNodesInHoodList[ i ] == aRootTaskId ) // root doesn't need to be hoo'up
        continue;
      if( aNodesInHoodList[ i ] == Gatherer[ SelectGatherer[ i ] ] )  // gatherers don't need to be hoo'up
        continue;

      TorusXYZ iXYZ       = MakeTaskXYZ( aNodesInHoodList[ i ] );
      int Dist            = 99999999;
      int SelectNodeIndex = -1;

      int iDistToGatherer = iXYZ.ManhattanDistanceImaged( PartitionXYZ, MakeTaskXYZ( Gatherer[ SelectGatherer[ i ] ] ) );

      for(int j = 0; j < aNodesInHoodCount; j++ )
        {
        if( SelectGatherer[ j ] != SelectGatherer[ i ] )  // this take root and nodes with other gatherers out
          continue;
        if( j == i )  // can't link to self
          continue;

        TorusXYZ jXYZ = MakeTaskXYZ( aNodesInHoodList[ j ] );
        int d = iXYZ.ManhattanDistanceImaged( PartitionXYZ, jXYZ );
        if( d < Dist )
          {
          int jDistToGatherer = jXYZ.ManhattanDistanceImaged( PartitionXYZ, MakeTaskXYZ( Gatherer[ SelectGatherer[ j ] ] ) );
          if( jDistToGatherer < iDistToGatherer )
            {
            SelectNodeIndex = j;
            Dist = d;
            if( Dist == 0 )
              PLATFORM_ABORT(" Found gatherer trying to link " );
            }
          }
        }
      CG.AddEdge( aNodesInHoodList[ i ], aNodesInHoodList[ SelectNodeIndex ] );
      }
    }

    static int HoodCastPacketLens[ 9 + 1 ];
    static int HoodRedPacketLens [ 9 + 1 ];

class CommGraphThrobber
  {
  public:
    // *** BEGIN INTERNAL BUFFER STRUCTURES ***
    double padding0[4]; 

    #define ATOM_XYZ_PER_PACKET_LIMIT (9)
    #define MAX_FRAGMENTS_PER_PACKET (4)

    struct HoodCastPacket
      {
      _BGL_TorusPktHdr mPacketHeader;
      int              mThisPacketId;
      short              mRootTaskId;
      unsigned char      mIsKey;    // The 'key packet is the one that updates counter, etc.
      unsigned char      mNumberOfAtoms;
      short              mNumberOfPacketsThisTime;
      short              mNumberOfFrags;
      unsigned short   mFragIds[ MAX_FRAGMENTS_PER_PACKET ];  // As soon as this and everything before it doesn't fit in header, might as well be 8
      XYZ              mPosits[ ATOM_XYZ_PER_PACKET_LIMIT ];
      };


    struct HoodRedPacket
      {
      _BGL_TorusPktHdr mPacketHeader;
      int              mThisPacketId;
      int              mRootTaskId;
      int              mNumberOfAtoms;
      // FragIds should not really need to ride back - could use the space for perf info.
      unsigned short   mFragIds[ MAX_FRAGMENTS_PER_PACKET ];  // As soon as this and everything before it doesn't fit in header, might as well be 8
      XYZ              mForces[ATOM_XYZ_PER_PACKET_LIMIT];
      };

      struct SummationOrderChecker
        {
        XYZ              mForces[ATOM_XYZ_PER_PACKET_LIMIT];
        };

    struct WrappedHoodRedPacket
      {
      union
        {
        _BGL_TorusPkt                mTorusPacket;
        HoodRedPacket                mHRP;
        };
      int                          mNumberOfReducePacketsToBeReceived;
#if !defined(BASIC_REDUCTION)
      XYZ mForceLowerBits[ATOM_XYZ_PER_PACKET_LIMIT] ;
#endif      
      };

    struct WrappedHoodCastPacket
      {
      union
        {
        _BGL_TorusPkt              mTorusPacket;
        HoodCastPacket             mHCP;
        };
      };

    struct FragDataRef
      {
      XYZ * mPositPtr;
      XYZ * mForcePtr;
      int   mSiteCount;
      };

    int mMaxFragId;
    int mMaxFragmentsInNeighborhood;
    int mAssignedFragsCleared;

    // MAX_FRAGMENT_ID ... max frag ids
    FragDataRef * mFragDataRef ;
    int         * mReceivedFragmentList ;
    int           mReceivedFragmentNextIndex ;


    int mFragDataRefBound;
    int mReceivedFragmentListBound;

// The torus packet struct is big enough and has alignment attributes
typedef _BGL_TorusPkt THROBBER_CACHE_PADDING;

    struct PerThreadData
      {
      THROBBER_CACHE_PADDING tcp0;
      int mCore;

      unsigned Raw_HoodCastPacketHeap;
      unsigned Raw_HoodRedWrappedPacketHeap;
      unsigned Raw_HoodRedWaitingPacketQueue;
      unsigned Raw_ReductionLocalPacketLocatorTable;

      _BGL_TorusPkt         * HoodCastPacketHeap;
      WrappedHoodRedPacket  * HoodRedWrappedPacketHeap;
      WrappedHoodRedPacket ** HoodRedWaitingPacketQueue;
      WrappedHoodRedPacket ** ReductionLocalPacketLocatorTable;

      // NOTE: these two fields change every time there is a ClearLocalFrags() call.
      int mUntunedNodesToHearFrom; // thread's assigned nodes in neighborhood before culling
      int mTunedNodesToHearFrom;   // how many nodes the thread will hear from after tuning out -1s

      int mMaxExpectedPackets;
      int mMaxFragId;

      THROBBER_CACHE_PADDING tcp1;

      PerThreadData()
        {
        mMaxFragId = -1;
        }

      void
      Init( int aCore, int aMaxFragId, int aMaxExpectedPackets, int aUntunedNodesToHearFrom )
        {
        if( mMaxFragId != -1 ) PLATFORM_ABORT("mMaxFragId != -1 *** has this call already been made? ***");
        if( aMaxFragId < 0 ) PLATFORM_ABORT("aMaxFragId < 0");
        if( aMaxExpectedPackets < 0) PLATFORM_ABORT("aMaxFragmentsInNeighborhood < 0");

        mCore                    = aCore;
        mMaxFragId               = aMaxFragId + 10;
        mMaxExpectedPackets      = aMaxExpectedPackets + 10; // plus 10 is padding - need at least 6 to cover receive overrun
        mUntunedNodesToHearFrom = aUntunedNodesToHearFrom; // how many nodes will send to this core - need to know to term comms
        mTunedNodesToHearFrom   = aUntunedNodesToHearFrom;

        int CachePad   = sizeof(THROBBER_CACHE_PADDING);
        int CachePad2X = 2 * CachePad;

        BegLogLine(PKTTH_LOG_INIT)
          << "PerThreadData::Init() "
          << " aMaxFragId " << aMaxFragId
          << " aMaxExpectedPackets " << aMaxExpectedPackets
          << " CachePad " << CachePad
          << " CachPad2X " << CachePad2X
          << " aCore=" << aCore
          << " aUntunedNodesToHearFrom=" << aUntunedNodesToHearFrom 
          << EndLogLine;

        Raw_HoodCastPacketHeap = (unsigned) malloc( sizeof(_BGL_TorusPkt) * mMaxExpectedPackets + CachePad2X + CACHE_ISOLATION_PADDING_SIZE);
        if( ! Raw_HoodCastPacketHeap ) PLATFORM_ABORT("Failed to allocate Raw_HoodCastPacketHeap ");
        HoodCastPacketHeap = (_BGL_TorusPkt *) ((Raw_HoodCastPacketHeap + CachePad) & ~0x0000003F);
        bzero(  HoodCastPacketHeap, mMaxExpectedPackets * sizeof( _BGL_TorusPkt ) );

        Raw_HoodRedWrappedPacketHeap = (unsigned)  malloc( sizeof( WrappedHoodRedPacket ) * mMaxExpectedPackets + CachePad2X + CACHE_ISOLATION_PADDING_SIZE);
        if( ! Raw_HoodRedWrappedPacketHeap ) PLATFORM_ABORT("Failed to allocate Raw_HoodRedWrappedPacketHeap ");
        HoodRedWrappedPacketHeap = (WrappedHoodRedPacket*) ((Raw_HoodRedWrappedPacketHeap + CachePad) & ~0x0000003F);
        bzero(  HoodRedWrappedPacketHeap, mMaxExpectedPackets * sizeof( WrappedHoodRedPacket ) );

        Raw_HoodRedWaitingPacketQueue = (unsigned) malloc( sizeof(WrappedHoodRedPacket*) * mMaxExpectedPackets + CachePad2X  + CACHE_ISOLATION_PADDING_SIZE);
        if( ! Raw_HoodRedWaitingPacketQueue ) PLATFORM_ABORT("Failed to allocate Raw_HoodRedWaitingPacketQueue ");
        HoodRedWaitingPacketQueue = (WrappedHoodRedPacket**) ((Raw_HoodRedWaitingPacketQueue + CachePad) & ~0x0000003F);
        bzero(  HoodRedWaitingPacketQueue, mMaxExpectedPackets * sizeof( WrappedHoodRedPacket * ) );

        Raw_ReductionLocalPacketLocatorTable = (unsigned) malloc( sizeof( WrappedHoodRedPacket * ) * mMaxFragId + CachePad2X + CACHE_ISOLATION_PADDING_SIZE);
        if( ! Raw_ReductionLocalPacketLocatorTable ) PLATFORM_ABORT("Failed to allocate Raw_ReductionLocalPacketLocatorTable ");
        ReductionLocalPacketLocatorTable = (WrappedHoodRedPacket**) ((Raw_ReductionLocalPacketLocatorTable + CachePad) & ~0x0000003F);
        bzero(  ReductionLocalPacketLocatorTable, sizeof( WrappedHoodRedPacket * ) * mMaxFragId );

        }

      };

    PerThreadData PTD[ THROBBER_THREAD_COUNT ];
      
    double padding1[4]; 
    // Is expected that this constructer will be newed into a static pointer.
    CommGraphThrobber()
      {
      mMaxFragId                            = -1;
      mMaxFragmentsInNeighborhood           = -1;
      mAssignedFragsCleared                 =  1;

      mFragDataRef                          = NULL;
      mReceivedFragmentList                 = NULL;
      mReceivedFragmentNextIndex            = -1;

      mNextInputPacketIndex                 = 0;
      mNextInputPacketXYZIndex              = 0;  // how many xyzs have been allocated
      mNextInputPacketFragIndex             = 0;  // frags

      mBroadcastReceivedPacketCount         = 0;

      for( int t = 0; t < THROBBER_THREAD_COUNT; t++ )
        mBroadcastReceivedPacketCountForThread[ t ] = 0;

      mFragDataRefBound                     = -1;
      mReceivedFragmentListBound            = -1;
      }

    void
    SetupCommThreadStructures( int        aMaxFragId,
                               int        aMaxFragmentsInNeighborhood,
                               CommGraph &CG )
      {
      if( mMaxFragId != -1 ) PLATFORM_ABORT("mMaxFragId != -1 *** has this call already been made? ***");
      if( aMaxFragId < 0 ) PLATFORM_ABORT("aMaxFragId < 0");
      if( aMaxFragmentsInNeighborhood < 0) PLATFORM_ABORT("aMaxFragmentsInNeighborhood < 0");

      if(THOOD_VERBOSE) printf("% 4d %12.8f :: SetupCommThreadStructures() MaxFragId % 4d\n",
                         Rank, MPI_Wtime(), aMaxFragId );

      mMaxFragId                  = aMaxFragId + 10; // plust 10 is padding - need at least 6
      mMaxFragmentsInNeighborhood = aMaxFragmentsInNeighborhood + 10;
      mAssignedFragsCleared       = 1;

      int NodesToHearFrom[ THROBBER_THREAD_COUNT ];
      bzero( (void*) NodesToHearFrom, THROBBER_THREAD_COUNT * sizeof( int) );
      NodesToHearFrom[ 0 ] = 1; // This node hears from itself on core 0
      assert( CG.mAddressSpaceCount > 0 );
      assert( Rank >= 0 && Rank <= CG.mAddressSpaceCount );
      for( int n = 0; n < CG.mAddressSpaceCount; n++)
        {
        if( n == Rank )
          continue;  // self send assigned to core 0 above

        if( CG.mTaskIdToGraphRecordMap[ n ] ) // this node needs to hear from all nodes in CG structs
          {
          NodesToHearFrom[ n % THROBBER_THREAD_COUNT ]++ ;
          }
        }

      for( int t = 0; t < THROBBER_THREAD_COUNT; t++ )
        PTD[ t ].Init( t, aMaxFragId, aMaxFragmentsInNeighborhood, NodesToHearFrom[t] );
      ////////////PTD[ 1 ].Init( 1, aMaxFragId, aMaxFragmentsInNeighborhood, NodesToHearFrom[1] );

      // Allocate the structure array that will keep track of packets for
      // both bcast reduce.
      mFragDataRefBound = mMaxFragId;
      // mFragDataRef      = new FragDataRef[ mFragDataRefBound ];
      mFragDataRef = (FragDataRef *) malloc( mFragDataRefBound * sizeof( FragDataRef ) + CACHE_ISOLATION_PADDING_SIZE);
      assert( mFragDataRef );
      memset( (void*) mFragDataRef, 0xFF, mFragDataRefBound * sizeof( FragDataRef ) );

      // Allocate the structure that will keep track of the fragments that are
      // in the hood and received by the broadcast.
      mReceivedFragmentListBound = mMaxFragId;
      // mReceivedFragmentList      = new int [ mReceivedFragmentListBound ];
      mReceivedFragmentList      = (int *) malloc( sizeof( int ) * mReceivedFragmentListBound + CACHE_ISOLATION_PADDING_SIZE);
      assert( mReceivedFragmentList );
      bzero( (void*) mReceivedFragmentList, mReceivedFragmentListBound * sizeof( int ) );

      ClearLocalFrags();  // Setup to pack frags into packets.

      if(THOOD_VERBOSE) printf("% 4d %12.8f :: SetupCommThreadStructures() Returning\n",
                         Rank, MPI_Wtime() );
      }

    XYZ *
    GetFragForcePtr( int FragId )
      {
      assert( FragId >= 0 && FragId < mMaxFragId );
      assert( mFragDataRef );
      XYZ * ForcePtr      = mFragDataRef[ FragId ].mForcePtr;
      assert( ForcePtr );
      return( ForcePtr );
      }

    XYZ *
    GetFragPositPtr( int FragId )
      {
      assert( FragId >= 0 && FragId < mMaxFragId );
      assert( mFragDataRef );
      XYZ * PositPtr      = mFragDataRef[ FragId ].mPositPtr;
      assert( PositPtr );
      return( PositPtr );
      }

    void
    SetGlobalFragInfo( int aFragId, int aAtomsInFragment )
      {
      assert( mMaxFragId >= 0 );
      assert( aFragId >= 0 && aFragId < mMaxFragId );
      assert( mFragDataRef );
      mFragDataRef[ aFragId ].mSiteCount = aAtomsInFragment;
      }

    int mNextInputPacketIndex;
    int mNextInputPacketXYZIndex;  // how many xyzs have been allocated
    int mNextInputPacketFragIndex;  // frags

    int mBroadcastReceivedPacketCountForThread[ THROBBER_THREAD_COUNT ];

    int mBroadcastReceivedPacketCount;
    int mTotalFragmentsReceivedOnBroadcast;

    void
    ClearLocalFrags()
      {
      assert( mMaxFragId >= 0 );
      assert( mNextInputPacketIndex <  mMaxFragId );
      assert( mNextInputPacketIndex >= 0 );

      mNextInputPacketIndex     = 0;
      mNextInputPacketXYZIndex  = 0;
      mNextInputPacketFragIndex = 0;

      // Setup the first packet so in the event that this node bcasts no frags
      // this one packet will be a viable contribution to the collective from this node.
      HoodCastPacket * BCastPkt = (HoodCastPacket *) &(PTD[ 0 ].HoodCastPacketHeap[ 0 ]);
      BCastPkt->mRootTaskId     = Rank;

      // Use application provided partition wide unique number system of FragIds - Id each packet with first frag in.
      BCastPkt->mThisPacketId   = -1 ; // This tag will result in no reduction for this packet
      BCastPkt->mNumberOfFrags  = 0 ;

      BCastPkt->mIsKey          = 1;
      BCastPkt->mNumberOfAtoms  = 0;
      BCastPkt->mNumberOfPacketsThisTime = 1 ;

      // Set the flag the shows that this is an assignment step.
      mAssignedFragsCleared = 1;

      BegLogLine( PKTTH_LOG_IF ) << "PktThrobber::ClearLocalFrags(): Done." << EndLogLine;
      }

    void
    SetFragAsLocal( int aFragId ) // This frag will be rooted on this node
      {
      assert( mAssignedFragsCleared );  // if this is not set, this is not an assignment ts

      if( aFragId < 0 || aFragId > mMaxFragId )
        PLATFORM_ABORT("SetFragAsLocal(): aFragId out of range ");

      assert( mFragDataRef[ aFragId ].mSiteCount );
      int PositCount = mFragDataRef[ aFragId ].mSiteCount;

      // Check the rules for advancing to fill the next packet
      if(   ( mNextInputPacketXYZIndex + PositCount) > ATOM_XYZ_PER_PACKET_LIMIT
          ||  mNextInputPacketFragIndex             == MAX_FRAGMENTS_PER_PACKET )
        {
        mNextInputPacketIndex  ++ ;

        mNextInputPacketXYZIndex  = 0;
        mNextInputPacketFragIndex = 0;

        HoodCastPacket * BCastPkt = (HoodCastPacket *) &(PTD[ 0 ].HoodCastPacketHeap[ mNextInputPacketIndex ]);
        BCastPkt->mRootTaskId     = Rank;

        BCastPkt->mNumberOfFrags  = 0 ;
        BCastPkt->mNumberOfAtoms  = 0 ;

        BCastPkt->mIsKey          = 0;

        // Increment the number of packets this time counter in the first packet
        HoodCastPacket * BCastFirstPkt = (HoodCastPacket *) &(PTD[ 0 ].HoodCastPacketHeap[ 0 ]);
        BCastFirstPkt->mNumberOfPacketsThisTime ++ ;
        }

      HoodCastPacket       * BCastPkt = (HoodCastPacket *)       &(PTD[ 0 ].HoodCastPacketHeap      [ mNextInputPacketIndex ]);
      WrappedHoodRedPacket * WRedPkt  = (WrappedHoodRedPacket *) &(PTD[ 0 ].HoodRedWrappedPacketHeap[ mNextInputPacketIndex ]);
      HoodRedPacket        *  RedPkt  = (HoodRedPacket *) &( WRedPkt->mTorusPacket );

      BegLogLine(PKTTH_LOG_ASSIGN)
        << "SetFragAsLocal()"
        << " aFragId "     << aFragId
        << " PktInd "      << mNextInputPacketIndex
        << " IsKey "       << BCastPkt->mIsKey
        << " PositCnt "    << PositCount
        << " NextXYZInd "  << mNextInputPacketXYZIndex
        << " NextFragInd " << mNextInputPacketFragIndex
        << EndLogLine;

      // Use application provided partition wide unique number system of FragIds - Id each packet with first frag in.
      // Note that this will overwrite the -1 of the key packet when the first frag is assigned.
      if( mNextInputPacketFragIndex == 0 )
        {
        BCastPkt->mThisPacketId   = aFragId ;
        }

      BCastPkt->mNumberOfFrags ++ ;
      BCastPkt->mFragIds[ mNextInputPacketFragIndex ] = aFragId;
      mNextInputPacketFragIndex ++ ;

      mFragDataRef[ aFragId ].mPositPtr = &( BCastPkt->mPosits[ mNextInputPacketXYZIndex ] );
      mFragDataRef[ aFragId ].mForcePtr = &(   RedPkt->mForces[ mNextInputPacketXYZIndex ] );

      mNextInputPacketXYZIndex += PositCount ;
      BCastPkt->mNumberOfAtoms += PositCount ;

      }

    // *** END INTERNAL BUFFER STRUCTURES ***
    void
    Pack()
      {
      BegLogLine(PKTTH_LOG_ASSIGN)
        << "Pack(): Not in spi "
        << " PktInd "      << mNextInputPacketIndex
        << " NextXYZInd "  << mNextInputPacketXYZIndex
        << " NextFragInd " << mNextInputPacketFragIndex
        << EndLogLine;
      }

    void
    dcache_store()
      {
      //((ThrobbinHood * )mThrobber)->dcache_store();
      PLATFORM_ABORT("dcache_store() not implemented");
      }

    XYZ*
    GetFragPositPtrInHood( int aFragId )
      {
      return ( GetFragPositPtr( aFragId ) );
      PLATFORM_ABORT("GetFragPositPtrInHood() not implemented");
      }

    XYZ*
    GetFragPositPtrLocal( int aFragId )
      {
      return ( GetFragPositPtr( aFragId ) );
      PLATFORM_ABORT("GetFragPositPtrLocal() not implemented");
      }

    int
    GetFragsInHoodCount()
      {
      return( GetFragsInNeighborhoodCount() );
      PLATFORM_ABORT("GetFragsIdInHoodCount() not implemented");
      }

    int
    WillOtherNodeBroadcastToThisNode( int p )
      {
      int rc = CG.mTaskIdToGraphRecordMap[ p ] ? 1 : 0;
      return( rc );
      }

    CommGraph CG;

    void
    Init( int  aDimX, int aDimY, int aDimZ,
                         int* aNodesToSendTo,
                         int  aNodesToSendToCount,
                         int  aFragmentsInPartition,
                         int  aMaxLocalFragments,
                         int  aMaxFragmentsInNeighborhood )
      {
      BegLogLine(PKTTH_LOG_INIT)
        << "CommGraphThrobber::Init()"
        << " HereTaskId "          << Platform::Topology::GetAddressSpaceId()
        << " aNodesToSendToCount " << aNodesToSendToCount
        << " aFragsInPartition "   << aFragmentsInPartition
        << " aMaxLocalFrags "      << aMaxLocalFragments
        << " aMaxFragsInHood "     << aMaxFragmentsInNeighborhood
        << EndLogLine;

      int RootTaskId = Platform::Topology::GetAddressSpaceId();

      int * SortedList = (int *) malloc( aNodesToSendToCount * sizeof( int ) + CACHE_ISOLATION_PADDING_SIZE);
      if( SortedList == NULL ) PLATFORM_ABORT("Init() failed to alloc sorted list space");

      memcpy( SortedList, aNodesToSendTo, aNodesToSendToCount * sizeof( int ) );

      Platform::Algorithm::Qsort( SortedList, aNodesToSendToCount, sizeof( int ), CompareI );

      /// BegLogLine( 1 ) << "Init() Got Here." << EndLogLine;

#ifndef THROBBER_SKIN_TREE
      CreateCommmGraph_MostDirectRoute
#else
      CreateCommmGraph_SkinTree
#endif
                                      ( aDimX, aDimY, aDimZ,
                                        RootTaskId,
                                        SortedList,
                                        aNodesToSendToCount,
                                        CG );

      ///////BegLogLine( 1 ) << "Init() Got Here." << EndLogLine;

      CG.Init( aDimX * aDimY * aDimZ, aNodesToSendToCount );

      //////BegLogLine( 1 ) << "Init() Got Here." << EndLogLine;

      CG.ProcessEdges( aNodesToSendToCount );

      if( PKTTH_LOG_INIT )
      {  
        CG.DumpDenseCommGraph("Before A2A");
      }

      //////BegLogLine( 1 ) << "Init() Got Here." << EndLogLine;

      CG.All2All_Exchange();

      if( PKTTH_LOG_INIT )
      {  
         CG.DumpDenseCommGraph("After A2A");
      }
      /////BegLogLine( 1 ) << "Init() Got Here." << EndLogLine;

      SetupCommThreadStructures( aFragmentsInPartition,
                                 aMaxFragmentsInNeighborhood == -1 ? aFragmentsInPartition : aMaxFragmentsInNeighborhood,
                                 CG );  // to scan to see which cores receive from how many nodes


      /////BegLogLine( 1 ) << "Init() Got Here." << EndLogLine;

      for(int a = 0; a < ATOM_XYZ_PER_PACKET_LIMIT + 1 ; a++ )
        {
        // Calculate the true payload length of the packet to be sent.
        int SizeofPkt  = (int)(sizeof( HoodCastPacket) - sizeof( _BGL_TorusPktHdr ));
        int NotUsed    = (int)(sizeof( XYZ ) * ( ATOM_XYZ_PER_PACKET_LIMIT - a ));
        int TruePayloadLen = SizeofPkt - NotUsed;

        HoodCastPacket p;

        BegLogLine( PKTTH_LOG_INIT )
          << "Init(): HoodCastPacketLens[ " << a << " ] Bytes " << TruePayloadLen << " " << (((unsigned)&(p.mPosits[a])) - ((unsigned) &p)) - sizeof( _BGL_TorusPktHdr )
          << EndLogLine;

        if( TruePayloadLen < 0 )
          PLATFORM_ABORT("Init(): TruePayloadLen < 0");
        if( TruePayloadLen > 240)
          PLATFORM_ABORT("Init(): TruePayloadLen > 240");

        HoodCastPacketLens[ a ] = TruePayloadLen;


        int RedSizeofPkt  = (int)(sizeof( HoodRedPacket) - sizeof( _BGL_TorusPktHdr ));
        int RedNotUsed    = (int)(sizeof( XYZ ) * ( ATOM_XYZ_PER_PACKET_LIMIT - a ));
        int RedTruePayloadLen = RedSizeofPkt - RedNotUsed;


        HoodRedPacketLens [ a ] = RedTruePayloadLen;
        }

      BegLogLine( PKTTH_LOG_INIT ) << "Init() Done." << EndLogLine;
      }

    //***
    // NOTE: On broadcast and on reduce core 0 holds all the 'root' packets
    struct BroadcastCoreArgs
      {
      CommGraph    * CG;
      PerThreadData* PTD;
      int            SendCount;
      int          * ReceivedPacketCountPtr;
      };

    void
    BroadcastExecute( int aSimTick, int aAssignmentViolation )
      {
      // AssignmentViolation arg and AssignedFragsCleared flag are the same info and must match
      // AssignmnetViolation arg is depricated.

      BegLogLine(PKTTH_LOG_BCAST)
        << "BroadcastExecute() Called"
        << " aSimTick " << aSimTick
        << " GuardViolation "      << aAssignmentViolation
        << " mAssignedFragsCleared "   << mAssignedFragsCleared
        << " mNextInputPacketIndex "       << mNextInputPacketIndex
        << " mMaxFragmentsInNeighborhood " << mMaxFragmentsInNeighborhood
        << " NodesInHood " << CG.mAddressSpaceCount
        << EndLogLine;

//      CG.DumpDenseCommGraph("Starting BroadcastExecute") ;
      int SendCount;

      if( mNextInputPacketIndex == 0 && mNextInputPacketXYZIndex == 0 )
        {
        if( mAssignedFragsCleared )
          SendCount = mNextInputPacketIndex + 1;
        else
          SendCount = 0;
        }
      else
        {
        SendCount = mNextInputPacketIndex + 1;
        }

//      CG.DumpDenseCommGraph("Starting BroadcastExecute") ;

      static THROBBER_CACHE_PADDING tcp0;
      static int recvcnt0;
      static THROBBER_CACHE_PADDING tcp1;
      static int recvcnt1;
      static THROBBER_CACHE_PADDING tcp2;

      BroadcastCoreArgs BCA[2];

      BCA[0].CG                     = &CG;
      BCA[0].PTD                    = &PTD[ 0 ];
      BCA[0].SendCount              =  SendCount;
      BCA[0].ReceivedPacketCountPtr = & recvcnt0;

#if THROBBER_THREAD_COUNT > 1      
      BCA[1].CG                     = &CG;
      BCA[1].PTD                    = &PTD[ 1 ];
      BCA[1].SendCount              =  0;  // other cores handle no intial sends
      BCA[1].ReceivedPacketCountPtr = & recvcnt1;
#endif
      
//      CG.DumpDenseCommGraph("BCA set") ;

      #ifndef NO_THROBBER_BCAST_CULLING
      // Nodes have updated local frags - so we need to send to the whole neighborhood on this ts
      // After this ts, until next reassignment, we can forgo sending dummy pkts (PktId = -1 )
      if( mAssignedFragsCleared )
        {
        PTD[ 0 ].mTunedNodesToHearFrom = PTD[ 0 ].mUntunedNodesToHearFrom;
#if THROBBER_THREAD_COUNT > 1      
       PTD[ 1 ].mTunedNodesToHearFrom = PTD[ 1 ].mUntunedNodesToHearFrom;
#endif
        BegLogLine(PKTTH_LOG_BCAST)
           << "mAssignedFragsCleared PTD[0].mTunedNodesToHearFrom=" << PTD[ 0 ].mTunedNodesToHearFrom
#if THROBBER_THREAD_COUNT > 1      
           << " PTD[1].mTunedNodesToHearFrom=" << PTD[ 1 ].mTunedNodesToHearFrom
#endif
           << EndLogLine ;
        // NOTE: this will need to be published to other cores on recook and recook + 1 ts
        }
#endif

//      CG.DumpDenseCommGraph("After maybe culling") ;

      Platform::Control::Barrier();

#if THROBBER_THREAD_COUNT > 1
rts_dcache_evict_normal();

      int h = Platform::Coprocessor::Start( CommGraphThrobber::BroadcastCore, (void*) &(BCA[1]), sizeof( BroadcastCoreArgs ) );
#endif

//      CG.DumpDenseCommGraph("Before BroadcastCore") ;

      BroadcastCore( & BCA[0] );


#if THROBBER_THREAD_COUNT > 1
      Platform::Coprocessor::Join( h );

///rts_dcache_store(void *beg, void *end);
//second out on 512 - hangs - really doesn't like this one coming out
//seems to be ok for 16k even orb runs like sope
// This might be trouble but i can't think of why or THIS COULD BE TROUBLE!!!!
// again seems to need this
rts_dcache_evict_normal();
#endif

      BegLogLine(PKTTH_LOG_BCAST)
        << "BroadcastExecute(): "
        << " Core0 RecvCnt " << recvcnt0
        << " Core1 RecvCnt " << recvcnt1
        << " Core0 RecvCnt@ " << (void*) &recvcnt0
        << " Core1 RecvCnt@ " << (void*) &recvcnt1
        << EndLogLine;

      mBroadcastReceivedPacketCountForThread[ 0 ] = recvcnt0;
      mBroadcastReceivedPacketCountForThread[ 1 ] = recvcnt1;
      mBroadcastReceivedPacketCount = recvcnt0 + recvcnt1;

      mTotalFragmentsReceivedOnBroadcast = 0;
      mReceivedFragmentNextIndex         = 0; // indexs a list of frag ids received

      // stitch received bcast packets to their partner reduce packets for all threads.
      for( int t = 0; t < THROBBER_THREAD_COUNT; t++ )
        {
         BegLogLine(PKTTH_LOG_BCAST)
            << "PTD[" << t
            << "].mTunedNodesToHearFrom is " << PTD[ t ].mTunedNodesToHearFrom
            << EndLogLine ;
        for(int p = 0; p < mBroadcastReceivedPacketCountForThread[ t ]; p++ )
          {
          HoodCastPacket *  HPC       = (HoodCastPacket *) &(PTD[ t ].HoodCastPacketHeap[ p ]);
          BegLogLine(PKTTH_LOG_BCAST)
            << " BCAST RPKTS "
            << " Core "          << t
            << " PktInd "        << p
            << " / "             << mBroadcastReceivedPacketCountForThread[ t ]
            << " RootTaskId "    << HPC->mRootTaskId
            << " PktId "         << HPC->mThisPacketId
            << " IsKey "         << HPC->mIsKey
            << " NoPkts "        << HPC->mNumberOfPacketsThisTime
            << " NoFrags "       << HPC->mNumberOfFrags
            << " RecvFragIndex " << mReceivedFragmentNextIndex
            << EndLogLine;
          /// Add the self frags to the total number reported at the user interface
          mTotalFragmentsReceivedOnBroadcast += HPC->mNumberOfFrags;
          WrappedHoodRedPacket * WRedPkt      = (WrappedHoodRedPacket *) &(PTD[ t ].HoodRedWrappedPacketHeap[ p ]);
          HoodRedPacket        *  RedPkt      = (HoodRedPacket *) &( WRedPkt->mTorusPacket );
          RedPkt->mThisPacketId               = HPC->mThisPacketId;
          RedPkt->mRootTaskId                 = HPC->mRootTaskId;
          RedPkt->mNumberOfAtoms              = HPC->mNumberOfAtoms;
#ifndef NO_THROBBER_BCAST_CULLING
          if( HPC->mThisPacketId == -1 )
            {
            assert(HPC->mIsKey);
            PTD[ t ].mTunedNodesToHearFrom -- ;
            BegLogLine(PKTTH_LOG_BCAST)
              << "Culling, PTD[" << t
              << "].mTunedNodesToHearFrom now " << PTD[ t ].mTunedNodesToHearFrom
              << EndLogLine ;
            }
#endif
          // Copy out a list of the received fragment ids.
          // And, hoo'up the ponters to the data
          int XYZInd = 0;
          for( int f = 0; f < HPC->mNumberOfFrags; f++ )
            {
            assert( f >= 0 && f <  MAX_FRAGMENTS_PER_PACKET );
            int RecvedFragId = HPC->mFragIds[ f ] ;
            mReceivedFragmentList[ mReceivedFragmentNextIndex ] = RecvedFragId ;
            mFragDataRef[ RecvedFragId ].mPositPtr = &(HPC->mPosits[ XYZInd ]);
            mFragDataRef[ RecvedFragId ].mForcePtr = &(RedPkt->mForces[ XYZInd ]);
            RedPkt->mFragIds[ f ] = HPC->mFragIds[ f ];
            XYZInd += mFragDataRef[ RecvedFragId ].mSiteCount;
            // NEED TO LOOK AT WHERE THE FOLLOWING VALUE IS INCED FOR THE PREEXIZTING PACKTs
            mReceivedFragmentNextIndex++;
            }
          }
        }

      // Reset the flag... can't add more frags until ClearLocalFrags called again.
      mAssignedFragsCleared = 0;

      BegLogLine(PKTTH_LOG_BCAST)
        << "BroadcastExecute() Done."
        << " SimTick "             << aSimTick
        << " GuardViolation "      << aAssignmentViolation
        << " Total Packets Recvd " << mBroadcastReceivedPacketCount
        << " Total Frags Recvd "   << mReceivedFragmentNextIndex
        << EndLogLine;
// THIS IS A DEBUG BARRIER AND SHOULD NOT BE REQUIRED
//////Platform::Control::Barrier();

//rts_dcache_evict_normal();
      }

    int
    GetFragsInNeighborhoodCount()
      {
      return( mTotalFragmentsReceivedOnBroadcast );
      }

    int
    GetFragIdInNeighborhood( int aFragIter )
      {
      int FragId = mReceivedFragmentList[ aFragIter ];
      return( FragId );
      }

    static
    void *
    BroadcastCore( void * Args )
      {
      BroadcastCoreArgs* ArgsPtr = (BroadcastCoreArgs*) Args;

      PerThreadData*  aPTD        = ArgsPtr->PTD;
      int             aSendCount  = ArgsPtr->SendCount;
      int             aAllocCount = ArgsPtr->PTD->mMaxExpectedPackets;
      int*            aRecvCount  = ArgsPtr->ReceivedPacketCountPtr;
      int             aCore       = ArgsPtr->PTD->mCore;
      CommGraph*      aCG         = ArgsPtr->CG;

      _BGL_TorusPkt*  aPktHeap    = aPTD->HoodCastPacketHeap;

      // Local node is considered heard from - Core 0 always handles self pkts
      // Unless the node has been culled (SendCount == 0)
      int TotalNodesHeardFrom   = (aCore == 0) ? (aSendCount?1:0) : 0;
      int TotalPacketsReceived  = aSendCount;

      int CurSendPktIndex       = -1 ;          // Will be incremented to 0 on first past through loop
      int NextToRecvPacketIndex = aSendCount;   // Where to put the next packet received
      int TotalPacketsThisTime  = aSendCount;   // A running count of total expected packets based on counts in key packets

      HoodCastPacket *     CurSendPktPtr = NULL;
      _BGL_TorusFifoStatus FifoStatus;

      int FifoSelect = 0;

      int CurSendPkt_RootTaskId_LeafLinkCount = 0;
      int CurSendPktRootTaskId = -1;

      BegLogLine(PKTTH_LOG_BCAST)
        << "BroadcastCore(): "
        << " Entered "
        << " Core " << aCore
        << " SendCnt " << aSendCount
        << " aPTD->mTunedNodesToHearFrom " << aPTD->mTunedNodesToHearFrom
        << " aPTD->mUntunedNodesToHearFrom " << aPTD->mUntunedNodesToHearFrom
        << " @RecvCnt " << (void*)aRecvCount
        << " @PktHeap " << (void*)aPktHeap
        << EndLogLine;

//	 aCG->DumpDenseCommGraph("Starting BroadcastCore") ;
 	 int AddressSpaceCount=Platform::Topology::GetAddressSpaceCount() ;
#if COMMGRAPH_VERIFY_SRC
	   {
	      int SenderCount = 0 ;
	      for( int i=0;i<AddressSpaceCount;i+=1)
	         {
	      	  if( CommGraph::mRecordSizeRecvData[i] > 0 )
	      	    {
	      		  BegLogLine(PKTTH_LOG_BCAST)
	      		    << "CommGraph::mRecordSizeRecvData[" << i
	      		    << "]=" <<  CommGraph::mRecordSizeRecvData[i]
	      		    << EndLogLine ;
	      		  SenderCount += 1 ;
	      	    }
	         }
	      BegLogLine(PKTTH_LOG_BCAST)
	      	    << "SenderCount=" << SenderCount 
	      	    << EndLogLine ;
	         
      }
#endif
#if COMMGRAPH_VERIFY_DEST	   
	   char AddressSpaceSendMap[AddressSpaceCount] ;
	   bzero(AddressSpaceSendMap,AddressSpaceCount) ;
#endif      
      
      for(;;)
        {
        if( aCore == 0 )
          _ts_GetStatus0( & FifoStatus );
        else
          _ts_GetStatus1( & FifoStatus );

        int PrevThereRank = -1 ;
        for(;;)  // Send side - break out of this loop when there is nothing to send or for fairness to receive side
          {
          // This loop is entered when the current packet is fully processed.
          while(     ! CurSendPkt_RootTaskId_LeafLinkCount
                 && ((CurSendPktIndex + 1 ) < NextToRecvPacketIndex) )
            {
#if COMMGRAPH_VERIFY_DEST
       	    bzero(AddressSpaceSendMap,AddressSpaceCount) ;   	  
#endif
        	PrevThereRank = -1 ;
            CurSendPktIndex ++ ;

            CurSendPktPtr       = (HoodCastPacket *) &(aPktHeap[ CurSendPktIndex ]);
            assert( CurSendPktPtr );

            CurSendPktRootTaskId = CurSendPktPtr->mRootTaskId;

            assert( CurSendPktRootTaskId >= 0 );
            assert( CurSendPktRootTaskId < aCG->mAddressSpaceCount );

            // This value will be counted down as packets are forwarded to the leaves.
            CurSendPkt_RootTaskId_LeafLinkCount = aCG->GetRecordLeafLinkCount( CurSendPktRootTaskId );
            BegLogLine(PKTTH_LOG_BCAST)
              << "CurSendPktIndex=" << CurSendPktIndex
              << " CurSendPkt_RootTaskId_LeafLinkCount=" << CurSendPkt_RootTaskId_LeafLinkCount             
              << EndLogLine ;
            }

          // If the while loop didn't find something to send, break out of send-side loop.
          if( ! CurSendPkt_RootTaskId_LeafLinkCount  )
            break;

          _BGL_TorusFifo * NextFifoToSend;
          if( aCore == 0 ) NextFifoToSend = _ts_CheckClearToSendOnLink0( & FifoStatus, FifoSelect, 8   );
          else             NextFifoToSend = _ts_CheckClearToSendOnLink1( & FifoStatus, FifoSelect, 8   );

          FifoSelect++;
          if( FifoSelect == 3 )
            FifoSelect = 0;

          // If the selected fifo is full, break out
          if( NextFifoToSend == NULL )
            break;

          int      ThereRank = aCG->GetRecordLeafLinkTaskId( CurSendPktRootTaskId,
                                                             CurSendPkt_RootTaskId_LeafLinkCount ); // 1 based index
          TorusXYZ ThereXYZ  = MakeTaskXYZ( ThereRank );  // this could be pre-cooked out.

#if THROBBER_THREAD_COUNT > 1
                  int DestCore = (CurSendPktPtr->mRootTaskId % THROBBER_THREAD_COUNT); // dest core
#else
                  int DestCore = 0;
#endif

          BegLogLine(PKTTH_LOG_BCAST)
            << "BcastPktSend "
            << " Src "                          << Rank
            << " Dst "                          << ThereRank
            << " Core "                         << (CurSendPktPtr->mRootTaskId % THROBBER_THREAD_COUNT)
            << " CurSendPktPtr->[ PktRootTid "  << CurSendPktPtr->mRootTaskId
            << " NumPkts "                      << CurSendPktPtr->mNumberOfPacketsThisTime
            << " mIsKey "                       << CurSendPktPtr->mIsKey
            << " PktId "                        << CurSendPktPtr->mThisPacketId
            << " NumFrags "                     << CurSendPktPtr->mNumberOfFrags
            << " NumAtoms "                     << CurSendPktPtr->mNumberOfAtoms
            << " Len "                          << HoodCastPacketLens[ CurSendPktPtr->mNumberOfAtoms ]
            << " ]"
            << " NextLeafLinkToSend "           << CurSendPkt_RootTaskId_LeafLinkCount
            << EndLogLine;

#if COMMGRAPH_VERIFY_DEST
          assert(ThereRank >=0 ) ;
          assert(ThereRank < AddressSpaceCount) ;
          if( AddressSpaceSendMap[ThereRank] )
            {
        	  aCG->DumpDenseCommGraph("After finding commgraph broken") ;
        	  assert(0) ; // Sending a packet where it has already been sent is bad
            }
          AddressSpaceSendMap[ThereRank] = 1 ;
#endif
          assert( ThereRank != PrevThereRank ) ; // Sending the same packet to a node twice represents a misconfigured commgraph
          PrevThereRank = ThereRank ;
          BGLTorusMakeHdrAppChooseRecvFifo( // this method picks a core as it's last arg
                              &(CurSendPktPtr->mPacketHeader),
                               ThereXYZ.mX,
                               ThereXYZ.mY,
                               ThereXYZ.mZ,
                               Rank,                // arg 0
                               ThereRank,           // arg 1
                               HoodCastPacketLens[ CurSendPktPtr->mNumberOfAtoms ], // payload in bytes
                               DestCore );

          BGLTorusInjectPacketImageApp( NextFifoToSend, ( _BGL_TorusPkt* ) CurSendPktPtr );

          CurSendPkt_RootTaskId_LeafLinkCount--;

          // Tried taking this out and breaking after a single packet
          break;  // THIS BREAK MEANS ONE (1) SEND AT A TIME -- COULD BE MORE, BUT 1 IS FAIR
          }

        assert( TotalNodesHeardFrom    <= aPTD->mTunedNodesToHearFrom );

        if( (CurSendPktIndex + 1 ) == TotalPacketsThisTime  // could be the last packet if all nodes heard from
            &&
            CurSendPkt_RootTaskId_LeafLinkCount == 0        // no more leaves to send to on last packet
            &&
            TotalNodesHeardFrom    == aPTD->mTunedNodesToHearFrom // make sure we've heard from all nodes to make sure we're checking the total expected packets

          )
          {
          BegLogLine(PKTTH_LOG_BCAST)
            << " BCAST EXIT LOOP "
            << " TotalPacketsReceived "   << TotalPacketsReceived
            << " TotalNodesHeardFrom "    << TotalNodesHeardFrom
            << " aPTD->mTunedNodesToHearFrom " << aPTD->mTunedNodesToHearFrom
            << " CurSendPktIndex "        << CurSendPktIndex
            << " NextToRecvPacketIndex "  << NextToRecvPacketIndex
            << " TotalPacketsThisTime "   << TotalPacketsThisTime
            << EndLogLine;
          break; // HoodCast done.
          }

        if( 1 ) // Context of receive side
          {
          // Make sure we have room for 6 packets
          if( NextToRecvPacketIndex >= ( aAllocCount - 6) )
            PLATFORM_ABORT("ThrobbinHood : Broadcast receiver over ran allocated packet buffers.");

          // Get 0..6 packets onto heap
//Need to conditionalize it on F0 F1
          int PacketCount;
          if( aCore == 0 )
              PacketCount = _ts_PollF0_OnePassReceive_Put( & FifoStatus,
                                                           &(aPktHeap[ NextToRecvPacketIndex ]),
                                                           sizeof( aPktHeap[ NextToRecvPacketIndex ])  );
          else
              PacketCount = _ts_PollF1_OnePassReceive_Put( & FifoStatus,
                                                           &(aPktHeap[ NextToRecvPacketIndex ]),
                                                           sizeof( aPktHeap[ NextToRecvPacketIndex ])  );

          for( ;PacketCount ; PacketCount-- )
            {
            HoodCastPacket * RecvedPkt = (HoodCastPacket *) &(aPktHeap[ NextToRecvPacketIndex ]);

            BegLogLine(PKTTH_LOG_BCAST)
              << "BcastPktRecv "
              << " Dst "              << aPktHeap[ NextToRecvPacketIndex ].hdr.swh1.arg
              << " Src "              << aPktHeap[ NextToRecvPacketIndex ].hdr.swh0.arg0
              << " Core "                         << aCore
              << " CurRecvPktPtr->[ PktRootTid "  << RecvedPkt->mRootTaskId
              << " NumPkts "                      << RecvedPkt->mNumberOfPacketsThisTime
              << " mIsKey "                       << RecvedPkt->mIsKey
              << " PktId "                        << RecvedPkt->mThisPacketId
              << " NumFrags "                     << RecvedPkt->mNumberOfFrags
              << " NumAtoms "                     << RecvedPkt->mNumberOfAtoms
              << " ]"
              << " BurstPkt "              << PacketCount
              << " }"
              << " Rank " << Rank
              << EndLogLine;

            assert( Rank == aPktHeap[ NextToRecvPacketIndex ].hdr.swh1.arg );

            assert( aCore == (RecvedPkt->mRootTaskId % THROBBER_THREAD_COUNT) );

            TotalPacketsReceived  ++;
#if defined(COMMGRAPH_VERIFY_SRC)
            BegLogLine(PKTTH_LOG_BCAST)
              << "mRecordSizeRecvData[" << RecvedPkt->mRootTaskId
              << "]=" << CommGraph::mRecordSizeRecvData[RecvedPkt->mRootTaskId]
              << EndLogLine ;
            assert(CommGraph::mRecordSizeRecvData[RecvedPkt->mRootTaskId] > 0) ;
#endif        
            if( RecvedPkt->mIsKey )
              {
              TotalNodesHeardFrom ++;
              TotalPacketsThisTime += RecvedPkt->mNumberOfPacketsThisTime;

              BegLogLine(PKTTH_LOG_BCAST) 
                "Key from RootTaskId=" << RecvedPkt->mRootTaskId
                << EndLogLine ;
              assert( TotalNodesHeardFrom    <= aPTD->mTunedNodesToHearFrom );


              }

            assert(    RecvedPkt->mNumberOfFrags >= 0
                    && RecvedPkt->mNumberOfFrags <= MAX_FRAGMENTS_PER_PACKET  );

            // Move to next receive buffer in poll.  Note- not done on -1 type
            // Next to send will go until it catches up to this value.
            NextToRecvPacketIndex ++ ;

            BegLogLine(PKTTH_LOG_BCAST)
              << " BCAST AFTER RECVPKT "
              << " TotalPacketsReceived "   << TotalPacketsReceived
              << " TotalNodesHeardFrom "    << TotalNodesHeardFrom
              << " aPTD->mTunedNodesToHearFrom " << aPTD->mTunedNodesToHearFrom
              << " CurSendPktIndex "        << CurSendPktIndex
              << " NextToRecvPacketIndex "  << NextToRecvPacketIndex
              << " TotalPacketsThisTime "   << TotalPacketsThisTime
              << EndLogLine;
            }
          }

        } // End of hood cast loop

      *aRecvCount = NextToRecvPacketIndex;

// co_co server now does this before signaling join is ok
// Publish packets recived to core 0 - this could be much more focused

//if( aCore != 0 )
//  {
//  rts_dcache_evict_normal();
///  }

      BegLogLine(PKTTH_LOG_BCAST)
        << " BroadcastCore(): Done "
        << " aRecvCount "             << *aRecvCount
        << " TotalPacketsReceived "   << TotalPacketsReceived
        << " TotalNodesHeardFrom "    << TotalNodesHeardFrom
        << " aPTD->mTunedNodesToHearFrom " << aPTD->mTunedNodesToHearFrom
        << " CurSendPktIndex "        << CurSendPktIndex
        << " NextToRecvPacketIndex "  << NextToRecvPacketIndex
        << " TotalPacketsThisTime "   << TotalPacketsThisTime
        << EndLogLine;
      return NULL ;
     }
    //*** end broadcast block
    //*** bgein reduce block
    struct ReduceCoreArgs
      {
      static THROBBER_CACHE_PADDING tcp0;
      PerThreadData*      PTD;
      CommGraph*          CG;
      int                 RecvPktCount;
      static THROBBER_CACHE_PADDING tcp1;
      };

    void
    ReductionExecute( int aSimTick )
      {
      BegLogLine(PKTTH_LOG_REDUCE)
        << "ReduceExecute(): Entered "
        << " aSimTick " << aSimTick
        << EndLogLine;

      ReduceCoreArgs RCA[ THROBBER_THREAD_COUNT ];

      for( int t = 0; t < THROBBER_THREAD_COUNT; t++ )
        {
        RCA[ t ].PTD          = &(PTD[ t ]);
        RCA[ t ].CG           = &CG;
        RCA[ t ].RecvPktCount = mBroadcastReceivedPacketCountForThread[ t ];
        }

      Platform::Control::Barrier();

#if THROBBER_THREAD_COUNT > 1
rts_dcache_evict_normal();

      int h = Platform::Coprocessor::Start( CommGraphThrobber::ReduceCore, (void*) &(RCA[1]), sizeof( ReduceCoreArgs ) );
#endif

      ReduceCore( (void*) & (RCA[ 0 ]) );

#if THROBBER_THREAD_COUNT > 1
      Platform::Coprocessor::Join( h );
#endif

/// 3'd out rts_dcache_evict_normal();
      BegLogLine(PKTTH_LOG_REDUCE)
        << "ReduceExecute(): Exiting "
        << " aSimTick " << aSimTick
        << EndLogLine;
      }

    static
    void*
    ReduceCore( void* aArgs )
      {
////// first out on 512 ... works
/////rts_dcache_evict_normal();
      ReduceCoreArgs* aRCA = (ReduceCoreArgs*) aArgs;

      PerThreadData* aPTD          = aRCA->PTD; // per thread data
      CommGraph*     aCG           = aRCA->CG;
      int            aRecvPktCount = aRCA->RecvPktCount;
      int            aCore         = aPTD->mCore;

      BegLogLine(PKTTH_LOG_REDUCE)
        << "ReduceCore(): Entered "
        << "RecvPktCount " << aRecvPktCount
        << EndLogLine;

      //NOT REALLY CLEAR THIS BZERO IS REQUIRED
//////////////////?????      bzero( aPTD->HoodRedWaitingPacketQueue, aPTD->mMaxFragmentsInNeighborhood * sizeof( WrappedHoodRedPacket * ) );

      int HoodRedWaitingPacketQueueFirstUnsent  = 0 ;
      int HoodRedWaitingPacketQueueLastUnsent   = 0 ; // This will be set up in the loop below to show sends for non-root packest.
      int HoodRedTotalPacketsToReceive          = 0 ; // This will be set up in the loop below to have total number of packets to be received to complete reduce

/// This could be avoided by zero when done using a location in the packet locator table
//////      bzero( (void*)aPTD->ReductionLocalPacketLocatorTable , aPTD->mMaxFragId * sizeof(WrappedHoodRedPacket *) );
      #ifndef NDEBUG
//// this check doesn't work. not sure why.  but we regress without it ... just make sure the map updated when it should be
      // Now this the locater pointer is NULLed below as the reduction on that packet finishes on this node
//?      for( int i = 0; i < aPTD->mMaxFragId; i++ )
//?        assert( aPTD->ReductionLocalPacketLocatorTable[ i ] == NULL );
      #endif

      
      // place the local packets into the send Q
      for( int i = 0; i < aRecvPktCount; i++ )
        {
        HoodRedPacket  * mHRP = (HoodRedPacket * ) &(aPTD->HoodRedWrappedPacketHeap[ i ].mTorusPacket);
        // Don't put -1 packet ids into the send Q
        // They will not be reduced.  The root needs to know not to wait for them.
        if( mHRP->mThisPacketId != -1 )
          {
          assert( mHRP->mThisPacketId >=0 && mHRP->mThisPacketId < aPTD->mMaxFragId );

          // This table enables us to join an incoming reduction packet with it's
          // local copy based on the PacketId
          aPTD->ReductionLocalPacketLocatorTable[ mHRP->mThisPacketId ]
                                          = & ( aPTD->HoodRedWrappedPacketHeap[ i ] );

          // get the count of packets that need to fold into this packet before it can be forwarded
          aPTD->HoodRedWrappedPacketHeap[ i ].mNumberOfReducePacketsToBeReceived
                                          = aCG->GetRecordLeafLinkCount( mHRP->mRootTaskId );

          HoodRedTotalPacketsToReceive += aPTD->HoodRedWrappedPacketHeap[ i ].mNumberOfReducePacketsToBeReceived;
#if !defined(BASIC_REDUCTION)
          WrappedHoodRedPacket * LocalWrappedPacketRef= aPTD->HoodRedWrappedPacketHeap + i;
          HoodRedPacket * LocalPacketRef = (HoodRedPacket *) &(LocalWrappedPacketRef->mTorusPacket);
          int numberOfAtoms = LocalPacketRef->mNumberOfAtoms;
    	  BegLogLine(PKTTH_LOG_REDUCE)
    	    << "i=" << i
    	    << " mHRP->mThisPacketId=" << mHRP->mThisPacketId 
    	    << EndLogLine ;
          for(int a = 0; a < numberOfAtoms; a++ )
            {
        	  LocalWrappedPacketRef->mForceLowerBits[a].Zero() ;
            }
#endif        
          // If this is a terminal leaf node, start the sends pointing to those packets from the send waiting queue.
          if( 0 == aPTD->HoodRedWrappedPacketHeap[ i ].mNumberOfReducePacketsToBeReceived )
            {
            aPTD->HoodRedWaitingPacketQueue[ HoodRedWaitingPacketQueueLastUnsent ]  = & ( aPTD->HoodRedWrappedPacketHeap[ i ] ) ;
            HoodRedWaitingPacketQueueLastUnsent ++;
            }

          BegLogLine(PKTTH_LOG_REDUCE)
            << "RED LOCAL"
            << " Ind "           << i
            << " / "             << aRecvPktCount
            << " PktRootTID "    << mHRP->mRootTaskId
            << " PktPktID "      << mHRP->mThisPacketId
            << " LeafCnt "       << (int)aPTD->HoodRedWrappedPacketHeap[ i ].mNumberOfReducePacketsToBeReceived
            << " TotPktsToRecv " << HoodRedTotalPacketsToReceive
            << " PktUsentQueue " << HoodRedWaitingPacketQueueLastUnsent
            << "PktForce mY[0..3] "   << mHRP->mForces[0].mY << " " << mHRP->mForces[1].mY << " " << mHRP->mForces[2].mY << " " << mHRP->mForces[3].mY
            << EndLogLine;
          }
        }

      int TotalPacketsReceived = 0;
      int SendPacketsWaiting   = ( HoodRedWaitingPacketQueueFirstUnsent < HoodRedWaitingPacketQueueLastUnsent );

      BegLogLine(PKTTH_LOG_REDUCE)
        << "RED START"
        << " HoodRedTotalPacketsToReceive "         << HoodRedTotalPacketsToReceive
        << " HoodRedWaitingPacketQueueFirstUnsent " << HoodRedWaitingPacketQueueFirstUnsent
        << " HoodRedWaitingPacketQueueLastUnsent "  << HoodRedWaitingPacketQueueLastUnsent
        << EndLogLine;

      _BGL_TorusFifoStatus FifoStatus;
      int RoundRobinFifoSelect = 0;
      // Reduce kernel
      for(;;)
        {
        if( aCore == 0 )
          _ts_GetStatus0( & FifoStatus );
        else
          _ts_GetStatus1( & FifoStatus );


        #if FIFO_COUNTERS
          aPTD->mTotalFifoPollsReduce ++ ;
        #endif

        while( SendPacketsWaiting )
          {
          WrappedHoodRedPacket * WrappedHoodRedPacketPtr
                                    = aPTD->HoodRedWaitingPacketQueue[ HoodRedWaitingPacketQueueFirstUnsent ];

          HoodRedPacket  * mHRP = (HoodRedPacket * ) &(WrappedHoodRedPacketPtr->mTorusPacket);

          assert( mHRP->mThisPacketId >=0 && mHRP->mThisPacketId < aPTD->mMaxFragId );

          int RootTaskId            = mHRP->mRootTaskId;

          // should ask about correct packet size rather than 8
          _BGL_TorusFifo * NextFifoToSend;
          if( aCore == 0 ) NextFifoToSend = _ts_CheckClearToSendOnLink0( & FifoStatus, RoundRobinFifoSelect, 8   );
          else             NextFifoToSend = _ts_CheckClearToSendOnLink1( & FifoStatus, RoundRobinFifoSelect, 8   );

          RoundRobinFifoSelect++;
          if( RoundRobinFifoSelect == 3 )
            RoundRobinFifoSelect = 0;

          // Check if there is room in the fifo to send, if not, break out and try later.  Could be better.
          if( NextFifoToSend == NULL )
            break;

          int      ThereRank = aCG->GetRootBoundLinkTaskId( RootTaskId );
          TorusXYZ ThereXYZ  = MakeTaskXYZ( ThereRank );

#if THROBBER_THREAD_COUNT > 1
                  int DestCore = (ThereRank == mHRP->mRootTaskId) ? 0 : (mHRP->mRootTaskId % THROBBER_THREAD_COUNT);
#else
                  int DestCore = 0;
#endif

          BegLogLine(PKTTH_LOG_REDUCE)
            << "RED SEND "
            << "Unsent Queue Head/Tail " << HoodRedWaitingPacketQueueFirstUnsent << "/" << HoodRedWaitingPacketQueueLastUnsent
            << " ThereRank "   << ThereRank
            << " PktRootTID "  << mHRP->mRootTaskId
            << " PktPacketId " << mHRP->mThisPacketId
            << " Force[0]XYZ " << mHRP->mForces[ 0 ].mX << " " << mHRP->mForces[ 0 ].mY << " " << mHRP->mForces[ 0 ].mZ
            << EndLogLine;

          BGLTorusMakeHdrAppChooseRecvFifo( // this method picks a core as it's last arg
                             &(WrappedHoodRedPacketPtr->mTorusPacket.hdr),
                               ThereXYZ.mX,
                               ThereXYZ.mY,
                               ThereXYZ.mZ,
                               Rank,       // arg 0
                               ThereRank,  // arg 1
                               HoodRedPacketLens [ mHRP->mNumberOfAtoms ], // Payload in bytes
                               DestCore );

          BGLTorusInjectPacketImageApp(   NextFifoToSend,
                                        &(WrappedHoodRedPacketPtr->mTorusPacket) );

          HoodRedWaitingPacketQueueFirstUnsent ++ ;
          SendPacketsWaiting = ( HoodRedWaitingPacketQueueFirstUnsent < HoodRedWaitingPacketQueueLastUnsent );

          //// break;  // ONE OUT AT A TIME
          }

        if( 1 ) // Context of receive side
          {
          // Get 0..6 packets onto stack buffer
          _BGL_TorusPkt SixPackets[ 6 ];

          int PacketCount;
          if( aCore == 0 )
              PacketCount = _ts_PollF0_OnePassReceive_Put( & FifoStatus,
                                                           &( SixPackets[ 0 ] ),
                                                           sizeof( _BGL_TorusPkt )  );
          else
              PacketCount = _ts_PollF1_OnePassReceive_Put( & FifoStatus,
                                                           &( SixPackets[ 0 ] ),
                                                           sizeof( _BGL_TorusPkt )  );


          for( int PacketIndex = 0; PacketIndex < PacketCount ;  PacketIndex++ )
            {
            HoodRedPacket  * HoodRedPacketBufferPtr = (HoodRedPacket * ) &( SixPackets[ PacketIndex ] );
            assert( HoodRedPacketBufferPtr );

            BegLogLine(PKTTH_LOG_REDUCE)
              << "RED RECV"
              << " Ind "         << PacketIndex
              << " Arg0 "        << SixPackets[ PacketIndex ].hdr.swh0.arg0
              << " Arg1 "        << SixPackets[ PacketIndex ].hdr.swh1.arg
              << " PktRootTID "  << HoodRedPacketBufferPtr->mRootTaskId
              << " PktPacketID " << HoodRedPacketBufferPtr->mThisPacketId
              << " Force[0] "    << HoodRedPacketBufferPtr->mForces[ 0 ].mX << " " << HoodRedPacketBufferPtr->mForces[ 0 ].mY << " " << HoodRedPacketBufferPtr->mForces[ 0 ].mZ
              << EndLogLine;

            // Find the local copy of this packet - this could be via a pointer in the incoming packet if we use a training phase
            // or the app can provide each packet with a uniq ordinal id  -- for example, MD could use first frag id as packet id
            // or we could hash.
            int packetId = HoodRedPacketBufferPtr->mThisPacketId ;
            assert( HoodRedPacketBufferPtr->mThisPacketId >= 0);
            assert( HoodRedPacketBufferPtr->mThisPacketId < aPTD->mMaxFragId );

            WrappedHoodRedPacket * LocalWrappedPacketRef =
                                     aPTD->ReductionLocalPacketLocatorTable[ HoodRedPacketBufferPtr->mThisPacketId ] ;

            assert( LocalWrappedPacketRef );

            HoodRedPacket * LocalPacketRef = (HoodRedPacket *) &(LocalWrappedPacketRef->mTorusPacket);

            assert( LocalPacketRef );

            assert( LocalPacketRef->mRootTaskId   == HoodRedPacketBufferPtr->mRootTaskId );
            assert( LocalPacketRef->mThisPacketId == HoodRedPacketBufferPtr->mThisPacketId );

            assert( LocalPacketRef->mRootTaskId >=0 && LocalPacketRef->mRootTaskId < aCG->mAddressSpaceCount );

            assert( LocalPacketRef->mFragIds[ 0 ]   == HoodRedPacketBufferPtr->mFragIds[ 0 ] );

            // this is the 'theta' op -- OR
            // should probably use templates to select code for the exact number of XYZs.
            int numberOfAtoms = LocalPacketRef->mNumberOfAtoms ;
	      	  BegLogLine(PKTTH_LOG_REDUCE)
	      	    << "mThisPacketId=" << HoodRedPacketBufferPtr->mThisPacketId 
	      	    << " numberOfAtoms=" << numberOfAtoms
	      	    << EndLogLine ;
	    	for(int a = 0; a < numberOfAtoms; a++ )
              {
#if !defined(BASIC_REDUCTION)
	    		BegLogLine(PKTTH_LOG_REDUCE)
	                << "Before quadsum mForces[" << a 
	                << "] = " << HoodRedPacketBufferPtr->mForces[ a ]
	        	    << " sum_mForces[" << a 
	        	    << "] = " << LocalPacketRef->mForces[a]
	        	    << " sum_mForceLowerBits[" << a
	        	    << "] = " << LocalWrappedPacketRef->mForceLowerBits[a]
	        	    << EndLogLine ;
	    		quadfloat_integrate(
	    			LocalPacketRef->mForces[ a ].mX,
	    			LocalWrappedPacketRef->mForceLowerBits[a].mX,
	    			LocalPacketRef->mForces[ a ].mX,
	    			LocalWrappedPacketRef->mForceLowerBits[a].mX,
	    			HoodRedPacketBufferPtr->mForces[ a ].mX
	    				) ;
	    		quadfloat_integrate(
	    			LocalPacketRef->mForces[ a ].mY,
	    			LocalWrappedPacketRef->mForceLowerBits[a].mY,
	    			LocalPacketRef->mForces[ a ].mY,
	    			LocalWrappedPacketRef->mForceLowerBits[a].mY,
	    			HoodRedPacketBufferPtr->mForces[ a ].mY
	    				) ;
	    		quadfloat_integrate(
	    			LocalPacketRef->mForces[ a ].mZ,
	    			LocalWrappedPacketRef->mForceLowerBits[a].mZ,
	    			LocalPacketRef->mForces[ a ].mZ,
	    			LocalWrappedPacketRef->mForceLowerBits[a].mZ,
	    			HoodRedPacketBufferPtr->mForces[ a ].mZ
	    				) ;
	    		BegLogLine(PKTTH_LOG_REDUCE)
	                << "After quadsum  mForces[" << a 
	                << "] = " << HoodRedPacketBufferPtr->mForces[ a ]
	        	    << " sum_mForces[" << a 
	        	    << "] = " << LocalPacketRef->mForces[a]
	        	    << " sum_mForceLowerBits[" << a
	        	    << "] = " << LocalWrappedPacketRef->mForceLowerBits[a]
	        	    << EndLogLine ;
	    		
#else
        	  LocalPacketRef->mForces[ a ] += HoodRedPacketBufferPtr->mForces[ a ] ;
#endif        	  
              }

            assert( LocalWrappedPacketRef->mNumberOfReducePacketsToBeReceived > 0 );

            LocalWrappedPacketRef->mNumberOfReducePacketsToBeReceived -- ;

            if( 0 == LocalWrappedPacketRef->mNumberOfReducePacketsToBeReceived  )
              {
              // Zero the location in the packet locator table for next time
              //?aPTD->ReductionLocalPacketLocatorTable[ HoodRedPacketBufferPtr->mThisPacketId ] = NULL;
              //? why bother to zero? it's reset if it's in use

                if(  LocalPacketRef->mRootTaskId != Rank )  // If there is further to go to root.
                {
                _BGL_TorusFifo * NextFifoToSend = NULL;
                #if REDUCE_EAGER_SEND
                  PLATFORM_ABORT("NOT HOOKED UP");
                  for( int i = 0; i < 3; i++ )
                    {
                    NextFifoToSend =
                             _ts_CheckClearToSendOnLink( & FifoStatus,
                                                         RoundRobinFifoSelect,  /////LinkToFifoMap[RootBoundLink], ///((RootBoundLink+1)%6) / 2,
                                                         8   ); // Chunks - NEED: calc this!!!
                    RoundRobinFifoSelect++;
                    if( RoundRobinFifoSelect == 3 )
                      RoundRobinFifoSelect = 0;
                    if( NextFifoToSend )
                      break;
                    }
                #endif

                if( NextFifoToSend ) // able to send
                  {
                  // almost certainly we can just send this packet
                  // eventually, when the packet is ready - well just send here.  for now, just queue the packet
                  int RootTaskId            = LocalPacketRef->mRootTaskId;

                  int      ThereRank = aCG->GetRootBoundLinkTaskId( RootTaskId );
                  TorusXYZ ThereXYZ  = MakeTaskXYZ( ThereRank );

                  HoodRedPacket * LocalPacketRef = (HoodRedPacket *) &(LocalWrappedPacketRef->mTorusPacket);

#if THROBBER_THREAD_COUNT > 1
                  int DestCore = (ThereRank == LocalPacketRef->mRootTaskId) ? 0 : (LocalPacketRef->mRootTaskId % THROBBER_THREAD_COUNT);
#else
                  int DestCore = 0;
#endif

                  BegLogLine(PKTTH_LOG_REDUCE)
                    << "RED SEND "
                    << "Unsent Queue Head/Tail " << HoodRedWaitingPacketQueueFirstUnsent << "/" << HoodRedWaitingPacketQueueLastUnsent
                    << " ThereRank "   << ThereRank
                    << " PktRootTID "  << LocalPacketRef->mRootTaskId
                    << " PktPacketId " << LocalPacketRef->mThisPacketId
                    << " Force[0]XYZ " << LocalPacketRef->mForces[ 0 ].mX << " " << LocalPacketRef->mForces[ 0 ].mY << " " << LocalPacketRef->mForces[ 0 ].mZ
                    << EndLogLine;

                  BGLTorusMakeHdrAppChooseRecvFifo( // this method picks a core as it's last arg
                                     &(LocalWrappedPacketRef->mTorusPacket.hdr),
                                       ThereXYZ.mX,
                                       ThereXYZ.mY,
                                       ThereXYZ.mZ,
                                       Rank,             // arg 0
                                       ThereRank,        // arg 1
                                       HoodRedPacketLens [ LocalPacketRef->mNumberOfAtoms ], ///// TruePayloadLen ); // Payload in bytes --  240 max
                                       DestCore );

                  BGLTorusInjectPacketImageApp( NextFifoToSend, &(LocalWrappedPacketRef->mTorusPacket) );

                  }
                else // not able to send - so queue up
                  {

                  aPTD->HoodRedWaitingPacketQueue[ HoodRedWaitingPacketQueueLastUnsent ] =  LocalWrappedPacketRef;
                  SendPacketsWaiting = 1;
                  HoodRedWaitingPacketQueueLastUnsent++;
                  }
                }
              }

            TotalPacketsReceived  ++;
            HoodRedTotalPacketsToReceive --;

            BegLogLine(PKTTH_LOG_REDUCE)
              << "RED SEND PHASE DONE"
              << " HoodRedTotalPacketsToReceive "        <<  HoodRedTotalPacketsToReceive
              << " SendPacketsWaiting "                  << SendPacketsWaiting
              << " TotalPacketsReceived "                << TotalPacketsReceived
              << " HoodRedWaitingPacketQueueLastUnsent " << HoodRedWaitingPacketQueueLastUnsent
              << EndLogLine;

            }

          }

	       if( HoodRedTotalPacketsToReceive == 0
	           &&
	           SendPacketsWaiting == 0 )
	          {
	          break; // done
	          }
        }

      return NULL ;
      }
    //*** end reduce block
  };


PktThrobberIF::PktThrobberIF()
  {
  mThrobber = NULL; // until Init is called
  BegLogLine(PKTTH_LOG_IF) << "PktThrobberIF::PktThrobberIF() Done." << EndLogLine;
  }

void
PktThrobberIF::dcache_store()
  {
  ((CommGraphThrobber * )mThrobber)->dcache_store();
  BegLogLine(PKTTH_LOG_IF) << "PktThrobberIF::dcache_store() Done." << EndLogLine;
  }

XYZ*
PktThrobberIF::GetFragPositPtrInHood( int aFragId )
  {
  XYZ* rc = ((CommGraphThrobber * )mThrobber)->GetFragPositPtr( aFragId ) ;
  BegLogLine(PKTTH_LOG_HIGH_FREQ) << "PktThrobberIF::GetFragPositPtrInHood() Done. aFragId = " << aFragId << " RetXYZ* " << rc << EndLogLine;
  return( rc );
  }

XYZ*
PktThrobberIF::GetFragPositPtrLocal( int aFragId )
  {
  XYZ* rc = ((CommGraphThrobber * )mThrobber)->GetFragPositPtrInHood( aFragId ) ;
  BegLogLine(PKTTH_LOG_HIGH_FREQ) << "PktThrobberIF::GetFragPositPtrLocal() Done. aFragId = " << aFragId <<  " RetXYZ* " << rc <<EndLogLine;
  return( rc );
  }

XYZ*
PktThrobberIF::GetFragForcePtr( int aFragId )
  {
  XYZ* rc = ((CommGraphThrobber * )mThrobber)->GetFragForcePtr( aFragId ) ;
  BegLogLine(PKTTH_LOG_HIGH_FREQ) << "PktThrobberIF::GetFragForcePtr() Done. aFragId = " << aFragId << " RetXYZ* " << rc << EndLogLine;
  return( rc );
  }

int
PktThrobberIF::GetFragIdInHood( int aFragIter )
  {
  int rc = ((CommGraphThrobber * )mThrobber)->GetFragIdInNeighborhood( aFragIter );
  BegLogLine(PKTTH_LOG_IF) << "PktThrobberIF::GetFragIdInHood() Done. aFragIter = " << aFragIter << " FragId " << rc << EndLogLine;
  return( rc );
  }

void
PktThrobberIF::SetGlobalFragInfo( int aFragId, int aSiteCount )
  {
  ((CommGraphThrobber * )mThrobber)->SetGlobalFragInfo( aFragId, aSiteCount );
  BegLogLine( 0 ) << "PktThrobberIF::SetGlobalFragInfo() Done. aFragId = " << aFragId << " SiteCount = " << aSiteCount << EndLogLine;
  }

void
PktThrobberIF::SetFragAsLocal( int aFragId )
  {
  ((CommGraphThrobber * )mThrobber)->SetFragAsLocal( aFragId );
  BegLogLine(PKTTH_LOG_HIGH_FREQ) << "PktThrobberIF::SetFragAsLocal() Done. aFragId = " << aFragId <<EndLogLine;
  }

void
PktThrobberIF::ClearLocalFrags()
  {
  ((CommGraphThrobber * )mThrobber)->ClearLocalFrags();
  BegLogLine(PKTTH_LOG_IF) << "PktThrobberIF::ClearLocalFrags() Done." << EndLogLine;
  }

int
PktThrobberIF::GetFragsInHoodCount()
  {
  int rc = ((CommGraphThrobber * )mThrobber)->GetFragsInNeighborhoodCount();
  BegLogLine(PKTTH_LOG_IF) << "PktThrobberIF::GetFragsInHoodCount() Done. FragsInHoodCount " << rc << EndLogLine;
  return( rc );
  }

void
PktThrobberIF::BroadcastExecute( int aSimTick, int aAssignmentViolation )
  {
  BegLogLine(PKTTH_LOG_IF)
    << "PktThrobberIF::BroadcastExecute() Called. "
    << " aSimTick "
    << aSimTick
    << "aAssViol "
    << aAssignmentViolation
    << EndLogLine;
  ((CommGraphThrobber * )mThrobber)->BroadcastExecute( aSimTick, aAssignmentViolation );
  BegLogLine(PKTTH_LOG_IF)
    << "PktThrobberIF::BroadcastExecute() Done. "
    << " aSimTick "
    << aSimTick
    << "aAssViol "
    << aAssignmentViolation
    << EndLogLine;
  }

void
PktThrobberIF::ReductionExecute( int aSimTick )
  {
  BegLogLine(PKTTH_LOG_IF) << "PktThrobberIF::ReductionExecute() Called. aSimTick " << aSimTick << EndLogLine;
  ((CommGraphThrobber * )mThrobber)->ReductionExecute( aSimTick );
  BegLogLine(PKTTH_LOG_IF) << "PktThrobberIF::ReductionExecute() Done. aSimTick " << aSimTick << EndLogLine;
  }

int
PktThrobberIF::WillOtherNodeBroadcastToThisNode( int p )
  {
  int rc = ((CommGraphThrobber * )mThrobber)->WillOtherNodeBroadcastToThisNode( p );
  BegLogLine( PKTTH_LOG_HIGH_FREQ ) << "PktThrobberIF::WillOtherNodeBroadcastToThisNode() Done. rc = " << rc << EndLogLine;
  return( rc );
  }

void
PktThrobberIF::Pack()
  {
  // Not much to say here yet.
  ((CommGraphThrobber * )mThrobber)->Pack();
  BegLogLine(PKTTH_LOG_HIGH_FREQ) << "PktThrobberIF::Pack() Done." << EndLogLine;
  return;
  }

int PktThrobberIF::DoesCulling()
  {
  return 0;
  }

void
PktThrobberIF::Init( int  aDimX, int aDimY, int aDimZ,
                     int* aNodesToSendTo,
                     int  aNodesToSendToCount,
                     int  aFragmentsInPartition,
                     int  aMaxLocalFragments,
                     int  aMaxFragmentsInNeighborhood )
    {

    Rank = Platform::Topology::GetAddressSpaceId();
    BegLogLine( PKFXLOG_PKTTHROBBER_INIT )
      << "PktThrobberIF::Init() called."
      << " Rank "                       << Rank
      << " Nodes in Neighborhood "      << aNodesToSendToCount
      << " FragmentsFragment (Id) "     << aFragmentsInPartition
      << " MaxLocalFragments "          << aMaxLocalFragments
      << " MaxFragmentsInNeighborhood " << aMaxFragmentsInNeighborhood
      << " THROBBER_THREAD_COUNT "      << THROBBER_THREAD_COUNT
      << EndLogLine;

    mThrobber = (void *) new CommGraphThrobber();

    ((CommGraphThrobber * )mThrobber)->Init(  aDimX, aDimY, aDimZ,
                                              aNodesToSendTo,
                                              aNodesToSendToCount,
                                              aFragmentsInPartition,
                                              aMaxLocalFragments,
                                              aMaxFragmentsInNeighborhood );

    BegLogLine(PKTTH_LOG_IF)
      << "PktThrobberIF::Init() Done."
      << EndLogLine;
    }
