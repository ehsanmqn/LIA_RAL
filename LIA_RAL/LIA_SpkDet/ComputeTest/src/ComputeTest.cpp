// ComputeTest.cpp
// This file is a part of LIA Software LIA_SpkDet, based on ALIZE toolkit 
// LIA_SpkDet  is a free, open tool for speaker recognition
// LIA_SpkDet is a development project initiated and funded by the LIA lab.
// See www.lia.univ-avignon.fr
// 
// ALIZE is needed for LIA_SpkDet
// for more information about ALIZE, see http://www.lia.univ-avignon.fr/heberges/ALIZE/
//
// Copyright (C) 2004
//  Laboratoire d'informatique d'Avignon [www.lia.univ-avignon.fr]
//  Jean-Francois Bonastre [jean-francois.bonastre@lia.univ-avignon.fr]
//      
// LIA_SpkDet is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The LIA team as well as the ALIZE project want to highlight the limits of voice authentication
// in a forensic context. 
// The following paper proposes a good overview of this point:
// [Bonastre J.F., Bimbot F., Boe L.J., Campbell J.P., Douglas D.A., Magrin-chagnolleau I.,
//  Person  Authentification by Voice: A Need of Caution,
//  Eurospeech 2003, Genova]
// The conclusion of the paper of the paper is proposed bellow:
// [Currently, it is not possible to completely determine whether the
//  similarity between two recordings is due to the speaker or to other
//  factors, especially when: (a) the speaker does not cooperate, (b) there
//  is no control over recording equipment, (c) recording conditions are not 
//  known, (d) one does not know whether the voice was disguised and, to a
//  lesser extent, (e) the linguistic content of the message is not
//  controlled. Caution and judgment must be exercised when applying speaker
//  recognition techniques, whether human or automatic, to account for these
//  uncontrolled factors. Under more constrained or calibrated situations,
//  or as an aid for investigative purposes, judicious application of these
//  techniques may be suitable, provided they are not considered as infallible.
//  At the present time, there is no scientific process that enables one to
//  uniquely characterize a person=92s voice or to identify with absolute
//  certainty an individual from his or her voice.]
//
// Contact Jean-Francois Bonastre (jean-francois.bonastre@lia.univ-avignon.fr) for
// more information about the licence or the use of LIA_SpkDet
// First version 15/07/2004
// New version 23/02/2005
// FA version 8/12/2006 (NS)
 
#if !defined(ALIZE_ComputeTest_cpp)
#define ALIZE_ComputeTest_cpp
 
#include <iostream>
#include <fstream> 
#include <cassert> 
#include <cmath> 
#include <liatools.h>
#include "ComputeTest.h"
#include <sys/stat.h>

using namespace alize;
using namespace std;

// Compute the mean LLR between the test files and the target models
// Input NDX Style format with lines "test ID1 ID2" 
// where test is the basename of the test file ID1 and ID2 the ID of the clients
// Output  NIST	style Format, depend on value of (segmentalMode)
// completeLLR - all the segments from the testfile are selected are used for computing one LLR by target
//               a line "gender ID decision test score" is added by target
// segmentLLR  - a LLR is computed for each segment of the testfile and each ID
//               a line "ge

int fexist(char *filename ) {
  struct stat buffer;
  if ( stat(filename, &buffer ) ) return 1 ;
  return 0 ;
}
// WindowLLR class - deals with LLR outputed by set of frames
class WindowLLR{
    bool _set;               // flag, indicates if the windowmode is on
    unsigned long _size;     // size of the window, in frames
    unsigned long _dec;      // shift of the window, in frames, gives the number of outputs
    unsigned long _nClient;  // number of different client, 1 by default;
    Matrix <double> *_llrM;  // contains the LLR for the window
    DoubleVector *_accLlrA;   // contains the accumulated LLR for the window
    ULongVector *_idxA;      // contains the idx of frames in the window
    unsigned long _bIdx;     // idx of first frame in the circular window
    unsigned long _count;    // nb of saved values in the circular window
    void _initMem();         // internal use, init the mem booking for score window
    void _freeMem();         // internal use, free the memory for

public:
    WindowLLR(Config &config); 
    ~WindowLLR();
    bool isSet(){return _set;}  
    void setNbClient(unsigned long nClient){_nClient=nClient;_initMem();}
    unsigned long getIdxBegin(){return (*_idxA)[_bIdx];}
    unsigned long getIdxEnd(){return (*_idxA)[(_bIdx+_count-1)%_size];}
    void showConfig();
    void accLLR(unsigned long clientIdx,double llr);
    double getLLR(unsigned long clientIdx);
    bool isEnd();
    unsigned long wCount(); // gives the number of data/frame in the window 
    void dec(unsigned long idxFrame);
};
// windowLLr functions
void WindowLLR::_initMem(){
    _freeMem();
    _idxA= new ULongVector(_size,_size);
    _accLlrA = new DoubleVector(_nClient,_nClient);
    _llrM= new Matrix <double>(_size,_nClient);
    for (unsigned long idxC=0;idxC<_nClient;idxC++){
	for (unsigned long idxF=0;idxF<_size;idxF++)
	    (*_llrM)(idxF,idxC)=0;
	(*_accLlrA)[idxC]=0;
    }
   
    _bIdx=0;
    _count=0;
}
void WindowLLR::_freeMem(){
    if (_llrM) {
	delete _llrM;
	delete _accLlrA;
	delete _idxA;
    }
    _llrM=NULL;
}
WindowLLR::WindowLLR(Config &config){
    _set=false;
    _size=0;
    _dec=0;
    _bIdx=0;
    _count=0;
    _nClient=0;
    _llrM=NULL;
    if (config.existsParam("windowLLR")) _set=config.getParam("windowLLR").toBool();
    if (_set){
	if (config.existsParam("windowLLRSize")) _size=config.getParam("windowLLRSize").toLong();
	else _size=30;
	if (config.existsParam("windowLLRDec")) _dec=config.getParam("windowLLRDec").toLong();
	else _dec=_size;	
	_nClient=1;
	_initMem();
    }
}
WindowLLR::~WindowLLR(){
    _freeMem();}
void WindowLLR::showConfig(){
    if (_set) cout<<"windowLLR mode size["<<_size<<"] dec["<<_dec<<"]"<<endl; 
}
unsigned long WindowLLR::wCount(){
    return (_count);
}
void WindowLLR::dec(unsigned long idxFrame){
    if (_count<_size){       //window is not full
	_count++;
	unsigned long eIdx=(_bIdx+_count-1)%_size;
	(*_idxA)[eIdx]=idxFrame;
    }
    else{// window is full, real dec (shift the window, step _dec frame)
	for (unsigned long wIdx=0;wIdx<_dec;wIdx++){
	    //suppress the begin value
	    for (unsigned long cIdx=0;cIdx<_nClient;cIdx++)
		(*_accLlrA)[cIdx]-=(*_llrM)(_bIdx,cIdx);	    	    
	    _bIdx=(_bIdx+1)%_size;
	}
	_count-=(_dec-1);
	(*_idxA)[(_bIdx+_count-1)%_size]=idxFrame;	
    }   
}
void WindowLLR::accLLR(unsigned long clientIdx,double llr){
    (*_llrM)((_bIdx+_count-1)%_size,clientIdx)=llr;
    (*_accLlrA)[clientIdx]+=llr;
}
double WindowLLR::getLLR(unsigned long clientIdx){
    return (*_accLlrA)[clientIdx]/(double)_size;
}   
bool WindowLLR::isEnd(){
    return (wCount()==_size);
}


//-------------------------------------------------------------------------------------------------------
int ComputeTest(Config& config)
{
  String inputNDXFileName = config.getParam("ndxFilename");                        // NDX inputfile filename - described the experience 
  String inputWorldFilename = config.getParam("inputWorldFilename");                   // World model file used for the LLR computation
  String outputNISTFileName = config.getParam("outputFilename");                       // Result file in NIST style (.nist) result file format
  String labelSelectedFrames =config.getParam("labelSelectedFrames");              // label for selected frames - Only the frames from segments 
                                                                                   // with this label  will be used
  bool segmentalMode=false;
  if (config.existsParam("segmentLLR")) segmentalMode=config.getParam("segmentLLR").toBool();  // selected mode for segmental computation (1 LLR by segment)
  String gender=config.getParam("gender");                                         // gives the gender for compatibility reasons with NIST standard output file
  WindowLLR windowLLR(config); // Initialize the windowLLR mode if requested
  windowLLR.showConfig();
  real_t decisionThreshold;
  if (config.existsParam("decisionThreshold"))                                     // Define the threshold if needed
    decisionThreshold=config.getParam("decisionthreshold").toDouble();
  else decisionThreshold=0;
  unsigned long maxClientLine=CST_MAX_CLIENT_LINE;                                 // Max of target Id for a ndx line
  if (config.existsParam("maxTargetLine")) maxClientLine=config.getParam("maxTargetLine").toLong();
  double frameLength = config.getParam("frameLength").toDouble();                  // length in s of a frame
  unsigned long nbMaxMixtureInMemory=0;
  if (config.existsParam("nbMaxMixtureInMemory")) nbMaxMixtureInMemory=config.getParam("nbMaxMixtureInMemory").toLong();
  unsigned long worldDecime=1;
  if (config.existsParam("worldDecime")) 
    worldDecime=config.getParam("worldDecime").toLong(); 
  if (verbose){
    cout << "Compute Test, Decime mode["<<worldDecime<<"] ";
    if (segmentalMode) cout << " Segmental Mode - 1 LLr by segment"<<endl;
    else cout << "File Mode- 1 LLR by file" <<endl;
  }
  try{
    XList ndx(inputNDXFileName,config);                                    // Read the test definition file (ndx)
    XLine *linep;                                                          // Pointor on the current test line
    ndx.getLine(0);
    MixtureServer ms(config);
    StatServer ss(config, ms);
    MixtureGD& world = ms.loadMixtureGD(inputWorldFilename);               // Load the world model
    TabClientLine tabClientLine(ms,config,maxClientLine); 	           // Initialize the client tab with 0 preload client
    ofstream outNist(outputNISTFileName.c_str(),ios::out | ios::trunc);    // Initialise the output file
    while ((linep=ndx.getLine()) != NULL){                                 // Loop on each line of the ndx input file
      String &featureFileName=linep->getElement(0);                        // Get the testfile basename
      FeatureServer fs(config,featureFileName);                            // Reading the feature file
      SegServer segmentsServer;                                                             // Create the segment server for managing the segments/clusters
      LabelServer labelServer;                                                              // Create the lable server, for indexing the segments/clusters
      initializeClusters(featureFileName,segmentsServer,labelServer,config);                // Reading the segmentation files for each feature input file
      verifyClusterFile(segmentsServer,fs,config);                                          // Verify if the segments ending before the end of the feature files...
      long codeSelectedFrame=labelServer.getLabelIndexByString(labelSelectedFrames);        // Get the index of the cluster with in interest audio segments
      if (codeSelectedFrame==-1)                                                            // The file is empty
	cout << "ATTENTION, TEST FILE ["<<featureFileName<<"] is empty"<<endl;
      else{
	SegCluster& selectedSegments=segmentsServer.getCluster(codeSelectedFrame); // Gives the cluster of the selected/used segments   
	TabClientLine tabClientLine(ms,config);      	                         // Load if needed the client models
	tabClientLine.loadLine(linep);
	if (windowLLR.isSet()) windowLLR.setNbClient(tabClientLine.nbClientLine()); // If needed reset the score/LLR window using the good number of client
	if (verbose)cout << "test seg["<<featureFileName<<"]"<< endl;
	MixtureGDStat &worldAcc=ss.createAndStoreMixtureGDStat(world);
	worldAcc.resetLLK();                                                               // Reset the world LLK accumulator
	RefVector <MixtureGDStat> clientAcc(tabClientLine.nbClientLine());
	for (unsigned int i=0; i<tabClientLine.nbClientLine();i++) {
		clientAcc.addObject(ss.createAndStoreMixtureGDStat(tabClientLine.getClientModel(i)));
		clientAcc[i].resetLLK();                                                           // Reset client i LLK accumulator
	}
	Seg* seg;                                                                         // reset the reader at the begin of the input stream
	selectedSegments.rewind();      
	while((seg=selectedSegments.getSeg())!=NULL){                                     // For each of the selected segments
	  unsigned long idxBeginFrame=seg->begin()+fs.getFirstFeatureIndexOfASource(seg->sourceName()); 
	  fs.seekFeature(idxBeginFrame); 
	  Feature f;
	  for (unsigned long idxFrame=0;idxFrame<seg->length();idxFrame++){               // For each frame of the segment
	      double llkw=0;
	      double llkc=0;
	      fs.readFeature(f);
	    if ((idxFrame%worldDecime)==0)                                                // We want to compute the world LLK and the top gaussian
		llkw=worldAcc.computeAndAccumulateLLK(f,1.0,DETERMINE_TOP_DISTRIBS);    // Determine the top components and compute wrld LLK
	    else worldAcc.computeAndAccumulateLLK(f,1.0,USE_TOP_DISTRIBS);       // Determine the top components and compute wrld LLK
	    if (windowLLR.isSet()) windowLLR.dec(idxBeginFrame+idxFrame);
	    for (unsigned long i=0;i<tabClientLine.nbClientLine();i++){                             // For each client, compute LLK using the winning
		llkc=clientAcc[i].computeAndAccumulateLLK(f,1.0,USE_TOP_DISTRIBS);
		if (windowLLR.isSet()) windowLLR.accLLR(i,llkc-llkw);
	    }
	    if (windowLLR.isSet() && windowLLR.isEnd()){
		for (unsigned long i=0;i < tabClientLine.nbClientLine();i++){                  // For each client
		    double LLRClient=windowLLR.getLLR(i);
		    char decision=setDecision(LLRClient,decisionThreshold);                     // take a decision 
		    outputResultLine(LLRClient, tabClientLine.getClientName(i),                 // Output the result
				     featureFileName, frameIdxToTime(windowLLR.getIdxBegin(),frameLength),frameIdxToTime(windowLLR.getIdxEnd(),frameLength),
				     gender ,decision,outNist);
		    if (verboseLevel>1) outputResultLine(LLRClient, tabClientLine.getClientName(i),                 // Output the result
							 featureFileName, frameIdxToTime(windowLLR.getIdxBegin(),frameLength),frameIdxToTime(windowLLR.getIdxEnd(),frameLength),
							 gender ,decision,cout);
		}  	
	    }
	  }  
	  if (segmentalMode){                                                             // A result is needed for each segment
	    double LLKWorld=worldAcc.getMeanLLK();                                        // Take the world LLK	
	    for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                  // For each client
		double LLKClient=clientAcc[i].getMeanLLK();                               // Get the mean LLK 
		double LLRClient=LLKClient-LLKWorld;                                      // Compute the LLR
		char decision=setDecision(LLRClient,decisionThreshold);                   // take a decision 
		outputResultLine(LLRClient, tabClientLine.getClientName(i),               // Output the result
				 featureFileName, frameIdxToTime(idxBeginFrame,frameLength),frameIdxToTime(idxBeginFrame+seg->length(),frameLength),
				 gender ,decision,outNist);
		if (verboseLevel>1) outputResultLine(LLRClient, tabClientLine.getClientName(i),             
						     featureFileName, frameIdxToTime(idxBeginFrame,frameLength),frameIdxToTime(idxBeginFrame+seg->length(),frameLength),
						     gender,decision ,cout);
		clientAcc[i].resetLLK();                                                   // Reset client i LLK accumulator
	    }  
	    worldAcc.resetLLK();                                                            // Reset the world LLK accumulator  
	  }
	}
	if (segmentalMode==false){                                                          // One result by file and by ID
	  double LLKWorld=worldAcc.getMeanLLK();                                                 // Take the world LLK
	  for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                             // For each client
	    double LLKClient=clientAcc[i].getMeanLLK();            // Get the mean LLK 		  
	    double LLRClient=LLKClient-LLKWorld;                                          // Compute the LLR
	    char decision=setDecision(LLRClient,decisionThreshold);                       // take a decision 
	    outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,outNist);
	    if (verboseLevel>1) {
	      outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,cout); 
	      if (verboseLevel>2) cout<<"** Client LLK:"<<LLKClient<<" UBM LLk:"<<LLKWorld<<endl;
	    }
	  }  
	}  
	clientAcc.deleteAllObjects();
      }
      if ((nbMaxMixtureInMemory>0)&&(ms.getMixtureCount()>nbMaxMixtureInMemory)){
	if (verbose) cout <<"Cleaning the mixture server from 1 to "<<ms.getMixtureCount()<<endl;
	ms.deleteMixtures(1,ms.getMixtureCount());  
	ms.deleteUnusedDistribs();
      } 
    }                                                                                   // end of the NDX line loop
    outNist.close();
  } // fin try
  catch (Exception& e){ 
    cout << e.toString().c_str() << endl;
  }
  return 0;
}

int ComputeTestFA(Config& config)
{ 
  String inputNDXFileName = config.getParam("ndxFilename");                        // NDX inputfile filename - described the experience 
  String inputWorldFilename = config.getParam("inputWorldFilename");                   // World model file used for the LLR computation
  String outputNISTFileName = config.getParam("outputFilename");                       // Result file in NIST style (.nist) result file format
  String labelSelectedFrames =config.getParam("labelSelectedFrames");              // label for selected frames - Only the frames from segments 
                                                                                   // with this label  will be used
  bool segmentalMode=(config.getParam("segmentalMode")=="segmentLLR");             // selected mode for segmental computation (1 LLR by file or by segment)
  String gender=config.getParam("gender");                                         // gives the gender for compatibility reasons with NIST standard output file
  real_t decisionThreshold;
  if (config.existsParam("decisionThreshold"))                                     // Define the threshold if needed
    decisionThreshold=config.getParam("decisionthreshold").toDouble();
  else decisionThreshold=0;
  unsigned long maxClientLine=CST_MAX_CLIENT_LINE;                                 // Max of target Id for a ndx line
  if (config.existsParam("maxTargetLine")) maxClientLine=config.getParam("maxTargetLine").toLong();
  //double frameLength = config.getParam("frameLength").toDouble();                  // length in s of a frame	  
  unsigned long nbMaxMixtureInMemory=1;
  if (config.existsParam("nbMaxMixtureInMemory")) nbMaxMixtureInMemory=config.getParam("nbMaxMixtureInMemory").toLong();
  unsigned long worldDecime=1;
  if (config.existsParam("worldDecime")) 
    worldDecime=config.getParam("worldDecime").toLong(); 
  if (config.getParam_debug())debug=true;  else debug=false;
    if (verbose){
    cout << "(Compute Test) Decime mode["<<worldDecime<<"] ";
    if (segmentalMode) cout << " Segmental Mode - 1 LLr by segment"<<endl;
    else cout << "File Mode- 1 LLR by file" <<endl;
    }
	if (verbose) cout << "(ComputeTest) Factor Analysis mode for channel effect" << endl; 

	    
 try{   
    XList ndx(inputNDXFileName,config);                                    // Read the test definition file (ndx)
    XLine *linep;                                                          // Pointor on the current test line
    ndx.getLine(0);
    MixtureServer ms(config);
    StatServer ss(config, ms);
    MixtureGD& world = ms.loadMixtureGD(inputWorldFilename);               // Load the world model
    TabClientLine tabClientLine(ms,config,maxClientLine); 	           // Initialize the client tab with 0 preload client	
    ofstream outNist(outputNISTFileName.c_str(),ios::out | ios::trunc);    // Initialise the output file
    
    while ((linep=ndx.getLine()) != NULL){                                 // Loop on each line of the ndx input file
      String &featureFileName=linep->getElement(0);                        // Get the testfile basename

	FeatureServer fs(config,featureFileName);                            // Reading the feature file		    
	/*TopGauss tg;
	tg.compute(world,fs,featureFileName,config);
	tg.write(featureFileName,config); // to remove but need by generalFAStats     	  */  
	FactorAnalysisStat FAAcc(featureFileName,fs,config);   	    
	FAAcc.estimateXYAndNorm(fs,config);
	cms(featureFileName,fs,config);

	if (verbose)cout << "---> LogLikelihood Ratio Computation of test segment ["<<featureFileName<<"]"<< endl;	
	TabClientLine tabClientLine(ms,config);      	                         // Load if needed the client models	
	tabClientLine.loadLine(linep);		

	/*********************
	Scoring with the TopGauss class, there is still a pb here
	*********************
	double llkw=tg.get(world,fs,featureFileName,config);	
	RealVector <double> llk;	
	llk.setSize(tabClientLine.nbClientLine());
	for (unsigned int i=0; i<tabClientLine.nbClientLine();i++)	
		llk[i]=tg.get(tabClientLine.getClientModel(i),fs,featureFileName,config);
	
	for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                             // For each client
		double LLRClient=llk[i]-llkw;                                          // Compute the LLR
		char decision=setDecision(LLRClient,decisionThreshold);                       // take a decision 
		if (verboseLevel>1){ 
			outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,cout); 
			cout<<"** C:"<<llk[i]<<" W:"<<llkw<<endl;	 
		}
		outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,outNist);
	}*/

	SegServer segmentsServer;                                                   
	LabelServer labelServer;   
	initializeClusters(featureFileName,segmentsServer,labelServer,config);    
	verifyClusterFile(segmentsServer,fs,config); 
	long codeSelectedFrame=labelServer.getLabelIndexByString(labelSelectedFrames);  
	SegCluster& selectedSegments=segmentsServer.getCluster(codeSelectedFrame); 
	
	MixtureGDStat &worldAcc=ss.createAndStoreMixtureGDStat(world);
	worldAcc.resetLLK();                                                               // Reset the world LLK accumulator
	RefVector <MixtureGDStat> clientAcc(tabClientLine.nbClientLine());
	for (unsigned int i=0; i<tabClientLine.nbClientLine();i++) {
		clientAcc.addObject(ss.createAndStoreMixtureGDStat(tabClientLine.getClientModel(i)));
		clientAcc[i].resetLLK();                                                           // Reset client i LLK accumulator
	}
	Seg* seg;                                                                         // reset the reader at the begin of the input stream
	selectedSegments.rewind();      
	while((seg=selectedSegments.getSeg())!=NULL){                                      // For each of the selected segments
	  unsigned long idxBeginFrame=seg->begin()+fs.getFirstFeatureIndexOfASource(seg->sourceName()); 
	  fs.seekFeature(idxBeginFrame); 
	  Feature f;
	  for (unsigned long idxFrame=0;idxFrame<seg->length();idxFrame++){               // For each frame of the segment
	      double llkw;
	      double llkc;
	      fs.readFeature(f);
	    if ((idxFrame%worldDecime)==0)                                                // We want to compute the world LLK and the top gaussian
		llkw=worldAcc.computeAndAccumulateLLK(f,1.0,DETERMINE_TOP_DISTRIBS);    // Determine the top components and compute wrld LLK
	    else worldAcc.computeAndAccumulateLLK(f,1.0,USE_TOP_DISTRIBS);       // Determine the top components and compute wrld LLK
	    for (unsigned long i=0;i<tabClientLine.nbClientLine();i++){                             // For each client, compute LLK using the winning
		llkc=clientAcc[i].computeAndAccumulateLLK(f,1.0,USE_TOP_DISTRIBS);
	    }
	  }  
	}
	if (segmentalMode==false){                                                          // One result by file and by ID
	  double LLKWorld=worldAcc.getMeanLLK();                                                 // Take the world LLK
	  for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                             // For each client
	    double LLKClient=clientAcc[i].getMeanLLK();            // Get the mean LLK 		  
	    double LLRClient=LLKClient-LLKWorld;                                          // Compute the LLR
	    char decision=setDecision(LLRClient,decisionThreshold);                       // take a decision 
	    outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,outNist);
	    if (verboseLevel>0) {
	      outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,cout); 
	      if (verboseLevel>0) cout<<"** Client LLK:"<<LLKClient<<" UBM LLk:"<<LLKWorld<<endl;
	    }
	  }  
	}  

	clientAcc.deleteAllObjects();
	
        if (verboseLevel > 2) cout << "To be deleted" <<  ms.toString() << endl;	  
        if ((nbMaxMixtureInMemory>0)&&(ms.getMixtureCount()>nbMaxMixtureInMemory)){
	if (verboseLevel >1) cout <<"Cleaning the mixture server from 1 to "<<ms.getMixtureCount()<<endl;
	ms.deleteMixtures(1,ms.getMixtureCount());  
	ms.deleteUnusedDistribs();
	if (verboseLevel > 2) cout << "deleted" <<  ms.toString() << endl;	  	
      } 
    }                                                                                   // end of the NDX line loop
    outNist.close(); 
  } // fin try
  catch (Exception& e){ 
    cout << e.toString().c_str() << endl;
  }
  return 0;
}

int ComputeTestNAP(Config& config)
{
  String inputNDXFileName = config.getParam("ndxFilename");                        // NDX inputfile filename - described the experience 
  String inputWorldFilename = config.getParam("inputWorldFilename");                   // World model file used for the LLR computation
  String outputNISTFileName = config.getParam("outputFilename");                       // Result file in NIST style (.nist) result file format
  String labelSelectedFrames =config.getParam("labelSelectedFrames");              // label for selected frames - Only the frames from segments 
                                                                                   // with this label  will be used
  bool segmentalMode=(config.getParam("segmentalMode")=="segmentLLR");             // selected mode for segmental computation (1 LLR by file or by segment)
  String gender=config.getParam("gender");                                         // gives the gender for compatibility reasons with NIST standard output file
  real_t decisionThreshold;
  if (config.existsParam("decisionThreshold"))                                     // Define the threshold if needed
    decisionThreshold=config.getParam("decisionthreshold").toDouble();
  else decisionThreshold=0;
  unsigned long maxClientLine=CST_MAX_CLIENT_LINE;                                 // Max of target Id for a ndx line
  if (config.existsParam("maxTargetLine")) maxClientLine=config.getParam("maxTargetLine").toLong();
  double frameLength = config.getParam("frameLength").toDouble();                  // length in s of a frame
  unsigned long nbMaxMixtureInMemory=1; //delete models
  if (config.existsParam("nbMaxMixtureInMemory")) nbMaxMixtureInMemory=config.getParam("nbMaxMixtureInMemory").toLong();
  unsigned long worldDecime=1;
  if (config.existsParam("worldDecime")) 
    worldDecime=config.getParam("worldDecime").toLong(); 
  if (config.getParam_debug())debug=true;  else debug=false;
    if (verbose){
    cout << "Compute Test, Decime mode["<<worldDecime<<"] ";
    if (segmentalMode) cout << " Segmental Mode - 1 LLr by segment"<<endl;
    else cout << "File Mode- 1 LLR by file" <<endl;
    }
    bool UBM=false;
    bool NAP=false;
    if (config.getParam("channelCompensation")=="NAP" && verbose) {cout << "NAP mode for channel effect" << endl; NAP=true;}
    else if (config.getParam("channelCompensation")=="UBM") {if (verbose) cout << "NAP mode for channel effect [W: add UBM channel effect]" << endl;UBM=true;}   
    else throw Exception("channelCompensation parameter is wrong",__FILE__,__LINE__);
	    
 try{   
    XList ndx(inputNDXFileName,config);                                    // Read the test definition file (ndx)
    XLine *linep;                                                          // Pointor on the current test line
    ndx.getLine(0);
    MixtureServer ms(config);
    StatServer ss(config, ms);
    MixtureGD& world = ms.loadMixtureGD(inputWorldFilename);               // Load the world model
    TabClientLine tabClientLine(ms,config,1); 	           // Initialize the client tab with 0 preload client	
    ofstream outNist(outputNISTFileName.c_str(),ios::out | ios::trunc);    // Initialise the output file
    
    Matrix<double> ChannelMatrix;
    ChannelMatrix.load(config.getParam("initChannelMatrix"),config); //get Channel Matrix from args and load in a Matrix object  
    if (verbose) cout<< "Removing channel effect from " << config.getParam("initChannelMatrix") << " of size: ["<< ChannelMatrix.rows() << "," <<ChannelMatrix.cols() << "]" << endl; 
    unsigned long svSize=world.getDistribCount()*ms.getVectSize();
    RealVector <double> channelVector(svSize,svSize);
    
    while ((linep=ndx.getLine()) != NULL){                                 // Loop on each line of the ndx input file
      String &featureFileName=linep->getElement(0);                        // Get the testfile basename
      MixtureGD &testModel=ms.createMixtureGD();    // create mixture for test model
      if (NAP) ms.loadMixture(testModel,featureFileName); // for nap, get test model
      FeatureServer fs(config,featureFileName);                            // Reading the feature file
      SegServer segmentsServer;                                                             // Create the segment server for managing the segments/clusters
      LabelServer labelServer;                                                              // Create the lable server, for indexing the segments/clusters
      initializeClusters(featureFileName,segmentsServer,labelServer,config);                // Reading the segmentation files for each feature input file
      verifyClusterFile(segmentsServer,fs,config);                                          // Verify if the segments ending before the end of the feature files...
      long codeSelectedFrame=labelServer.getLabelIndexByString(labelSelectedFrames);        // Get the index of the cluster with in interest audio segments
      if (codeSelectedFrame==-1)                                                            // The file is empty
	cout << "WARNING, TEST FILE ["<<featureFileName<<"] is empty"<<endl;
      else{
	SegCluster& selectedSegments=segmentsServer.getCluster(codeSelectedFrame); // Gives the cluster of the selected/used segments   
	TabClientLine tabClientLine(ms,config);      	                         // Load if needed the client models  
	tabClientLine.loadLine(linep);
	if (verboseLevel > 2) cout << "Load line :  " << ms.toString() << endl;
	for (unsigned int i=0; i<tabClientLine.nbClientLine();i++) {
			if (verbose) cout << "---> Compute Test Channel effect for: [" << featureFileName <<"]["<<tabClientLine.getClientModel(i).getId()<<"]"<<endl;
			if (UBM) computeNAPChannelEffect(world,tabClientLine.getClientModel(i),ChannelMatrix);				
			if (NAP) computeNAPChannelEffect(testModel,tabClientLine.getClientModel(i),ChannelMatrix);
	}
	if (verbose)cout << "---> LogLikelihood Ratio Computation of test segment ["<<featureFileName<<"]"<< endl;
	MixtureGDStat &worldAcc=ss.createAndStoreMixtureGDStat(world);
	worldAcc.resetLLK();                                                               // Reset the world LLK accumulator
	RefVector <MixtureGDStat> clientAcc(tabClientLine.nbClientLine());
	for (unsigned int i=0; i<tabClientLine.nbClientLine();i++) {
		clientAcc.addObject(ss.createAndStoreMixtureGDStat(tabClientLine.getClientModel(i)));
		clientAcc[i].resetLLK();                                                           // Reset client i LLK accumulator
	}
	
	Seg* seg;                                                                         // reset the reader at the begin of the input stream
	selectedSegments.rewind();      

	while((seg=selectedSegments.getSeg())!=NULL){                                     // For each of the selected segments
	  unsigned long idxBeginFrame=seg->begin()+fs.getFirstFeatureIndexOfASource(seg->sourceName()); 
	  fs.seekFeature(idxBeginFrame); 
	  Feature f;
	  for (unsigned long idxFrame=0;idxFrame<seg->length();idxFrame++){               // For each frame of the segment
	    fs.readFeature(f);
	    if ((idxFrame%worldDecime)==0)                                                // We want to compute the world LLK and the top gaussian
	      worldAcc.computeAndAccumulateLLK(f,1.0,DETERMINE_TOP_DISTRIBS);    // Determine the top components and compute wrld LLK
	    else worldAcc.computeAndAccumulateLLK(f,1.0,USE_TOP_DISTRIBS);       // Determine the top components and compute wrld LLK
	    for (unsigned int i=0;i<tabClientLine.nbClientLine();i++)                             // For each client, compute LLK using the winning
	      clientAcc[i].computeAndAccumulateLLK(f,1.0,USE_TOP_DISTRIBS);
	  }  
	  if (segmentalMode){                                                             // A result is needed for each segment
	    double LLKWorld=worldAcc.getMeanLLK();                                         // Take the world LLK	
	    for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                  // For each client
	      double LLKClient=clientAcc[i].getMeanLLK();            // Get the mean LLK 
	      double LLRClient=LLKClient-LLKWorld;                                        // Compute the LLR
	      char decision=setDecision(LLRClient,decisionThreshold);                     // take a decision 
	      outputResultLine(LLRClient, tabClientLine.getClientName(i),                 // Output the result
			       featureFileName, frameIdxToTime(idxBeginFrame,frameLength),frameIdxToTime(idxBeginFrame+seg->length(),frameLength),
			       gender ,decision,outNist);
	      if (verboseLevel>1) outputResultLine(LLRClient, tabClientLine.getClientName(i),             
					    featureFileName, frameIdxToTime(idxBeginFrame,frameLength),frameIdxToTime(idxBeginFrame+seg->length(),frameLength),
					    gender,decision ,cout);
	      clientAcc[i].resetLLK();                               // Reset client i LLK accumulator
	    }  
	    worldAcc.resetLLK();                                                           // Reset the world LLK accumulator  
	  }
	}
	if (segmentalMode==false){                                                        // One result by file and by ID
	  double LLKWorld=worldAcc.getMeanLLK();                                                 // Take the world LLK
	  for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                             // For each client
	    double LLKClient=clientAcc[i].getMeanLLK();            // Get the mean LLK 
	    double LLRClient=LLKClient-LLKWorld;                                          // Compute the LLR
	    char decision=setDecision(LLRClient,decisionThreshold);                       // take a decision 
	    outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,outNist);
	    if (verboseLevel>1) {
	      outputResultLine(LLRClient, tabClientLine.getClientName(i),featureFileName ,gender ,decision,cout); 
	      if (verboseLevel>2) cout<<"** C:"<<LLKClient<<" W:"<<LLKWorld<<endl;
	    }
	  }  
	}  
      }	
        if (verboseLevel > 2) cout << "To be deleted" <<  ms.toString() << endl;	  
        if ((nbMaxMixtureInMemory>0)&&(ms.getMixtureCount()>nbMaxMixtureInMemory)){
	if (verboseLevel >1) cout <<"Cleaning the mixture server from 1 to "<<ms.getMixtureCount()<<endl;
	ms.deleteMixtures(1,ms.getMixtureCount());  
	ms.deleteUnusedDistribs();
	if (verboseLevel > 2) cout << "deleted" <<  ms.toString() << endl;	  	
      } 
    }                                                                                   // end of the NDX line loop
    outNist.close();
  } // fin try
  catch (Exception& e){ 
    cout << e.toString().c_str() << endl;
  }
  return 0;
}


//-------------------------------------------------------------------------------------------------------
int ComputeTestByLabel(Config& config)
{
  String inputNDXFileName = config.getParam("ndxFileName");                        // NDX inputfile filename - described the experience 
  String inputWorldFilename = config.getParam("inputWorldFilename");                   // World model file used for the LLR computation
  String outputNISTFileName = config.getParam("outputFile");                       // Result file in NIST style (.nist) result file format
  //bool segmentalMode=(config.getParam("segmentalMode")=="segmentLLR");             // selected mode for segmental computation (1 LLR by file or by segment)
  String gender=config.getParam("gender");                                         // gives the gender for compatibility reasons with NIST standard output file
  real_t decisionThreshold;
  if (config.existsParam("decisionThreshold"))                                     // Define the threshold if needed
    decisionThreshold=config.getParam("decisionthreshold").toDouble();
  else decisionThreshold=0;
  unsigned long worldDecime=1;
  if (config.existsParam("worldDecime")) 
    worldDecime=config.getParam("worldDecime").toLong(); 
  unsigned long maxClientLine=CST_MAX_CLIENT_LINE;                                 // Max of target Id for a ndx line
  if (config.existsParam("maxTargetLine")) maxClientLine=config.getParam("maxTargetLine").toLong();
  bool useDefaultClientModel=true;                                                 // If a label model is missing, use the client model
  if (config.existsParam("useDefaultClientModel"))  useDefaultClientModel=(config.getParam("useDefaultClientModel")=="true");
  bool byUserRep=false;
  if (config.existsParam("byUserRep"))  byUserRep=(config.getParam("byUserRep")=="true");

  bool weigthedFusion=true;
  if (config.existsParam("weigthedFusion"))  weigthedFusion=(config.getParam("weigthedFusion")=="true");
  //double frameLength = config.getParam("frameLength").toDouble();                  // length in s of a frame : N.S. useless
  
  unsigned long nbMaxMixtureInMemory=0;
  if (config.existsParam("nbMaxMixtureInMemory")) nbMaxMixtureInMemory=config.getParam("nbMaxMixtureInMemory").toLong();
  bool verboseAll=false;
  if (config.existsParam("verboseAll"))verboseAll=true; else verboseAll=false;
  if (verbose){
    cout << "Compute Test -by label model option, world decime["<<worldDecime<<"]";
    if (useDefaultClientModel) cout <<" Use default client model if a label model is missing";
    if (byUserRep) cout <<" the label models are in a by user rep user/user_label.gmm";
      cout <<endl;
  }
  try{
    XList ndx(inputNDXFileName,config);                                    // Read the test definition file (ndx)
    XLine *linep;                                                          // Pointor on the current test line
    ndx.getLine(0);
    MixtureServer ms(config);
    StatServer ss(config, ms);
    MixtureGD& world = ms.loadMixtureGD(inputWorldFilename);               // Load the world model
    TabClientLine tabClientLine(ms,config,maxClientLine); 	           // Initialize the client tab with 0 preload client
    ofstream outNist(outputNISTFileName.c_str(),ios::out | ios::trunc);    // Initialise the output file
    while ((linep=ndx.getLine()) != NULL){                                 // Loop on each line of the ndx input file
      String &featureFileName=linep->getElement(0);                        // Get the testfile basename
      FeatureServer fs(config,featureFileName);                            // Reading the feature file
      SegServer segmentsServer;                                                             // Create the segment server for managing the segments/clusters
      LabelServer labelServer;                                                              // Create the lable server, for indexing the segments/clusters
      initializeClusters(featureFileName,segmentsServer,labelServer,config);                // Reading the segmentation files for each feature input file
      verifyClusterFile(segmentsServer,fs,config);                                          // Verify if the segments ending before the end of the feature files..
      TabClientLine tabClientLine(ms,config);                                         
      ScoreAccum accumScore;                                                                // The LLR accumulator for each client (label result fusion) 
      for (unsigned long codeSelectedFrame=0;codeSelectedFrame<segmentsServer.getClusterCount();codeSelectedFrame++){ // For each cluster
	String label=labelServer.getLabel(codeSelectedFrame).getString();                   // Get the label name for the current cluster
	if (verbose) cout <<"test seg["<<featureFileName<<"] Label ["<<label<<"]"<<endl;   
	if (tabClientLine.loadLine(linep,label,useDefaultClientModel,byUserRep)){           // Load if needed the client models by label  - return 0 if no model
	  SegCluster& selectedSegments=segmentsServer.getCluster(codeSelectedFrame);          // Gives the current cluster    
	  ss.resetLLK(world);                                                                 // Reset the world LLK accumulator
	  for (unsigned int i=0; i<tabClientLine.nbClientLine();i++)
	    ss.resetLLK(tabClientLine.getClientModel(i));                                     // Reset client i LLK accumulator
	  Seg* seg;                                                                           // reset the reader at the begin of the input stream
	  selectedSegments.rewind(); 
	  unsigned long nbFrame=0;
	  while((seg=selectedSegments.getSeg())!=NULL){                                       // For each of the selected segments
	    unsigned long idxBeginFrame=seg->begin()+fs.getFirstFeatureIndexOfASource(seg->sourceName()); 
	    fs.seekFeature(idxBeginFrame); 
	    Feature f;
	    nbFrame++;
	    for (unsigned long idxFrame=0;idxFrame<seg->length();idxFrame++){    // For each frame of the segment
	      fs.readFeature(f);
	      if ((idxFrame%worldDecime)==0)                                                // We want to compute the world LLK and the top gaussian
		ss.computeAndAccumulateLLK(world, f,DETERMINE_TOP_DISTRIBS);    // Determine the top components and compute wrld LLK
	      else ss.computeAndAccumulateLLK(world, f,USE_TOP_DISTRIBS);       // Determine the top components and compute wrld LLK
	      for (unsigned int i=0;i<tabClientLine.nbClientLine();i++)                              // For each client, compute LLK using the winning
		ss.computeAndAccumulateLLK(tabClientLine.getClientModel(i),f,USE_TOP_DISTRIBS);
	    }
	  }
	  double LLKWorld=ss.getMeanLLK(world);                                             // Take the world LLK	
	  for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                               // For each client
	    double LLKClient=ss.getMeanLLK(tabClientLine.getClientModel(i));                // Get the mean LLK 
	    double LLRClient=LLKClient-LLKWorld;                                            // Compute the LLR
	    char decision=setDecision(LLRClient,decisionThreshold);                         // take a decision 
	    outputResultLine(LLRClient, tabClientLine.getClientName(i)+"_"+label,featureFileName ,gender ,decision,outNist);
	    if (verboseAll) {
	      outputResultLine(LLRClient, tabClientLine.getClientName(i)+"_"+label,featureFileName ,gender ,decision,cout); 
	      cout<<"** C:"<<LLKClient<<" W:"<<LLKWorld<<endl;
	    }
	    if (weigthedFusion) accumScore.addAndAccumulate(tabClientLine.getClientName(i),LLRClient,nbFrame);
	    else accumScore.addAndAccumulate(tabClientLine.getClientName(i),LLRClient,1);
	  }
	} // end of test - no model in the line
      }                                                                                    // end of cluster loop (label loop)
      for (unsigned int i=0;i < accumScore.getSize();i++){                                          // For each client
	char decision=setDecision(accumScore.getScore(i),decisionThreshold);               // take a decision 
	outputResultLine(accumScore.getScore(i), accumScore.getId(i),featureFileName ,gender ,decision,outNist);
	if (verboseAll)outputResultLine(accumScore.getScore(i),accumScore.getId(i),featureFileName ,gender ,decision,cout);     
      }  
      if ((nbMaxMixtureInMemory>0)&&(ms.getMixtureCount()>nbMaxMixtureInMemory)){
	if (verbose) cout <<"Cleaning the mixture server from 1 to "<<ms.getMixtureCount()<<endl;
	ms.deleteMixtures(1,ms.getMixtureCount());  
	ms.deleteUnusedDistribs();
      }
    }                                                                                      // end of the NDX line loop
    outNist.close();
  } // fin try
  catch (Exception& e){ 
    cout << e.toString().c_str() << endl;
  }
  return 0;
}

//-------------------------------------------------------------------------------------------------------------
// Compute Histos and OutputEntropy
int ComputeTestHisto(Config& config)
{
  String inputNDXFileName = config.getParam("ndxFileName");                        // NDX inputfile filename - described the experience 
  String inputWorldFilename = config.getParam("inputWorldFilename");                   // World model file used for the LLR computation
  String outputNISTFileName = config.getParam("outputFile");                       // Result file in NIST style (.nist) result file format
  String labelSelectedFrames =config.getParam("labelSelectedFrames");              // label for selected frames - Only the frames from segments 
                                                                                   // with this label  will be used
  bool segmentalMode=(config.getParam("segmentalMode")=="segmentLLR");             // selected mode for segmental computation (1 LLR by file or by segment)
  String gender=config.getParam("gender");                                         // gives the gender for compatibility reasons with NIST standard output file
  real_t decisionThreshold;
  if (config.existsParam("decisionThreshold"))                                     // Define the threshold if needed
    decisionThreshold=config.getParam("decisionthreshold").toDouble();
  else decisionThreshold=0;
  unsigned long maxClientLine=CST_MAX_CLIENT_LINE;                                 // Max of target Id for a ndx line
  if (config.existsParam("maxTargetLine")) maxClientLine=config.getParam("maxTargetLine").toLong();
  unsigned long nbBins=config.getParam("nbBins").toLong();
  //double frameLength = config.getParam("frameLength").toDouble();                  // length in s of a frame
  unsigned long nbMaxMixtureInMemory=0;
  if (config.existsParam("nbMaxMixtureInMemory")) nbMaxMixtureInMemory=config.getParam("nbMaxMixtureInMemory").toLong();
  unsigned long worldDecime=1;
  if (config.existsParam("worldDecime")) 
    worldDecime=config.getParam("worldDecime").toLong(); 
  if (config.existsParam("debug"))debug=true;  else debug=false;
  if (config.existsParam("verbose"))verbose=true; else verbose=false;
  bool verboseAll=false;
  if (config.existsParam("verboseAll"))verboseAll=true; else verboseAll=false;
  String scoreType=config.getParam("scoreType");
  if (verbose){
    cout << "**** Likelihood Ratio will be computed with Histo ****" << endl;
	if (config.getParam("scoreType")=="entropy") {cout << "**** Scores are entropies ****" << endl;}
	if (config.getParam("scoreType")=="mean") {cout << "**** Robust Mean using Histo ****" << endl;}
    cout << "Compute Test, Decime mode["<<worldDecime<<"] ";
    if (segmentalMode) cout << " Segmental Mode - Confidences scores will be used to discard some segments"<<endl;
    else cout << "File Mode- 1 LLR by file" <<endl;
  }
  try{
    XList ndx(inputNDXFileName,config);                                    // Read the test definition file (ndx)
    XLine *linep;                                                          // Pointor on the current test line
    ndx.getLine(0);
    MixtureServer ms(config);
    StatServer ss(config, ms);
    MixtureGD& world = ms.loadMixtureGD(inputWorldFilename);               // Load the world model
    TabClientLine tabClientLine(ms,config,maxClientLine); 	           // Initialize the client tab with 0 preload client
    ofstream outNist(outputNISTFileName.c_str(),ios::out | ios::trunc);    // Initialise the output file
    while ((linep=ndx.getLine()) != NULL){                                 // Loop on each line of the ndx input file
      String &featureFileName=linep->getElement(0);                        // Get the testfile basename
      FeatureServer fs(config,featureFileName);                            // Reading the feature file
      SegServer segmentsServer;                                                             // Create the segment server for managing the segments/clusters
      LabelServer labelServer;                                                              // Create the lable server, for indexing the segments/clusters
      initializeClusters(featureFileName,segmentsServer,labelServer,config);                // Reading the segmentation files for each feature input file
      verifyClusterFile(segmentsServer,fs,config);                                          // Verify if the segments ending before the end of the feature files...
      long codeSelectedFrame=labelServer.getLabelIndexByString(labelSelectedFrames);        // Get the index of the cluster with in interest audio segments
      if (codeSelectedFrame==-1)                                                            // The file is empty
	cout << "ATTENTION, TEST FILE ["<<featureFileName<<"] is empty"<<endl;
      else{
	SegCluster& selectedSegments=segmentsServer.getCluster(codeSelectedFrame); // Gives the cluster of the selected/used segments   
	TabClientLine tabClientLine(ms,config);      	                         // Load if needed the client models
	tabClientLine.loadLine(linep);
	TabHisto tabHisto(nbBins,maxClientLine); // nb Bins to estimate pdfs and maxTargetLine in an ndx
	ScoreAccum accumModifiedLLR;
	double llk_w=0.0;
	double llk_c=0.0;
	if (verbose)cout << "test seg["<<featureFileName<<"]"<< endl;
	ss.resetLLK(world);                                                               // Reset the world LLK accumulator
	for (unsigned int i=0; i<tabClientLine.nbClientLine();i++)
	  ss.resetLLK(tabClientLine.getClientModel(i));                                   // Reset client i LLK accumulator
	Seg* seg;                                                                         // reset the reader at the begin of the input stream
	selectedSegments.rewind();      
	while((seg=selectedSegments.getSeg())!=NULL){                                     // For each of the selected segments
	  unsigned long idxBeginFrame=seg->begin()+fs.getFirstFeatureIndexOfASource(seg->sourceName()); 
	  fs.seekFeature(idxBeginFrame); 
	  Feature f;
	  for (unsigned long idxFrame=0;idxFrame<seg->length();idxFrame++){               // For each frame of the segment
	    fs.readFeature(f);
	    if ((idxFrame%worldDecime)==0)                                                // We want to compute the world LLK and the top gaussian
	llk_w=ss.computeAndAccumulateLLK(world, f,DETERMINE_TOP_DISTRIBS);    // Determine the top components and compute wrld LLK
	    else ss.computeAndAccumulateLLK(world, f,USE_TOP_DISTRIBS);       // Determine the top components and compute wrld LLK
	    for (unsigned int i=0;i<tabClientLine.nbClientLine();i++)  {                          // For each client, compute LLK using the winning
	llk_c=ss.computeAndAccumulateLLK(tabClientLine.getClientModel(i),f,USE_TOP_DISTRIBS);
	//cout <<tabClientLine.getClientName(i)<<" "<<llk_c<<" "<<llk_w<<" "<<llk_c-llk_w<<endl;
	tabHisto.accumulateValueInTab(tabClientLine.getClientName(i),llk_c-llk_w);}
		    }
	  if (segmentalMode){                                                             // A result is needed for each segment
	    double LLKWorld=ss.getMeanLLK(world);                                         // Take the world LLK	
	    for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                           // For each client
	      double LLKClient=ss.getMeanLLK(tabClientLine.getClientModel(i));            // Get the mean LLK 
	      double LLRClient=LLKClient-LLKWorld;                                        // Compute the LLR
	      //char decision=setDecision(LLRClient,decisionThreshold);                     // take a decision 
		Histo & currentHisto=tabHisto.getHisto(tabClientLine.getClientName(i));
		double entropy=computeEntropy(currentHisto);
		if (verboseAll) cout << "Segment: "<<tabClientLine.getClientName(i)<< " "<<seg->begin()<<":"<<seg->length()<<"  LLR: "<< LLRClient << " Entropy: " << entropy <<endl;
		//if (entropy < decisionThreshold) {
		if (0) {
		if (verboseAll) cerr << "Discard segment" << LLRClient << endl;
			LLRClient=0.0;}
		accumModifiedLLR.addAndAccumulate(tabClientLine.getClientName(i),LLRClient,1);
		currentHisto.computeHisto(0);

	      ss.resetLLK(tabClientLine.getClientModel(i));                               // Reset client i LLK accumulator
	    }  
	    ss.resetLLK(world);                                                           // Reset the world LLK accumulator  
	  }
	}
	
	if (segmentalMode) { // output the global score
		for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){ 
			double score=accumModifiedLLR.getScore(tabClientLine.getClientName(i));
			if (verboseAll) outputResultLine(score, tabClientLine.getClientName(i),featureFileName ,gender ,0,cout);
			outputResultLine(score, tabClientLine.getClientName(i),featureFileName ,gender ,0,outNist);
		}
	}
	if (segmentalMode==false){                                                        // One result by file and by ID
		double LLKWorld=ss.getMeanLLK(world);                                           // Take the world LLK
		for (unsigned int i=0;i < tabClientLine.nbClientLine();i++){                             // For each client
			tabHisto.computeHistoInTab(tabClientLine.getClientName(i));
			double LLKClient=ss.getMeanLLK(tabClientLine.getClientModel(i));              // Get the mean LLK 
			double LLRClient=LLKClient-LLKWorld;                                          // Compute the LLR
			char decision=setDecision(LLRClient,decisionThreshold);                       // take a decision 
			Histo & currentHisto=tabHisto.getHisto(tabClientLine.getClientName(i));
			if (debug) currentHisto.saveGnuplot(tabClientLine.getClientName(i)+".hist");
			double score=0.0;
			if(scoreType=="entropy") score=computeEntropy(currentHisto);
			else if(scoreType=="mean") score=computeMean(currentHisto);
			else {cerr << "Error: unknown score type!" << endl;}
			outputResultLine(score, tabClientLine.getClientName(i),featureFileName ,gender ,decision,outNist);
			if (verboseAll) {
				outputResultLine(score, tabClientLine.getClientName(i),featureFileName ,gender ,decision,cout); 
				cout<<"** C:"<<LLKClient<<" W:"<<LLKWorld<<" LR:"<<LLRClient<<endl;
			}
		}  
	} // end non segmental mode  
      }// end ndx loop
      if ((nbMaxMixtureInMemory>0)&&(ms.getMixtureCount()>nbMaxMixtureInMemory)){
	if (verbose) cout <<"Cleaning the mixture server from 1 to "<<ms.getMixtureCount()<<endl;
	ms.deleteMixtures(1,ms.getMixtureCount());  
	ms.deleteUnusedDistribs();
      } 
    }                                                                                   // end of the NDX line loop
    outNist.close();
  } // fin try
  catch (Exception& e){ 
    cout << e.toString().c_str() << endl;
  }
  return 0;
}


#endif //!defined(ALIZE_ComputeTest_cpp)
