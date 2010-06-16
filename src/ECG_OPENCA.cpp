/*
 ============================================================================
 Name        : ECG_OPENCA.cpp
 Author      : Vincent
 Copyright   : Your copyright notice
 Description : Main application program
 ============================================================================
 */

// INCLUDE FILES


//#include <windef.h>

#include <stdio.h>
#include <wfdb/wfdb.h>
//#include <wfdb/wfdblib.h>
#include <wfdb/ecgcodes.h>
#include <wfdb/ecgmap.h>


#include "qrsdet.h"		// For sample rate.

// This is a GCCE toolchain workaround needed when compiling with GCCE
// and using main() entry point
#ifdef __GCCE__
#include <staticlibinit_gcce.h>
#endif



//Our program
#define MITDB				// Comment this line out to process AHA data.
#ifdef MITDB
#define ECG_DB_PATH	"C:\\data\\others\\MITDB\\"	 // Path to where MIT/BIH data.
//#define ECG_DB_PATH	"C:/data/others/MITDB"	


//#define ECG_DB_PATH "D:\\S60\\devices\\S60_3rd_FP2_SDK_v1.1\\epoc32\\winscw\\c\\Data\\Others\\MITDB"   //1
//#define ECG_DB_PATH "D:/S60/devices/S60_3rd_FP2_SDK_v1.1/epoc32/winscw/c/Data/Others/MITDB"			//2
//#define ECG_DB_PATH "epoc32\\winscw\\c\\Data\\Others\\MITDB"//3
//#define ECG_DB_PATH "epoc32/winscw/c/Data/Others/MITDB"//4
//#define ECG_DB_PATH	"C:\\data\\others\\MITDB"//5
//#define ECG_DB_PATH	"C:/data/others/MITDB/"//6
//
//#define ECG_DB_PATH "D:\\S60\\devices\\S60_3rd_FP2_SDK_v1.1\\epoc32\\winscw\\c\\Data\\Others\\MITDB\\"   //7
//#define ECG_DB_PATH "D:/S60/devices/S60_3rd_FP2_SDK_v1.1/epoc32/winscw/c/Data/Others/MITDB/"			//8
//#define ECG_DB_PATH "epoc32\\winscw\\c\\Data\\Others\\MITDB\\"//9
//#define ECG_DB_PATH "epoc32/winscw/c/Data/Others/MITDB/"//10
#define REC_COUNT	48
int Records[REC_COUNT] = {100,101,102,103,104,105,106,107,108,109,111,112,
							113,114,115,116,117,118,119,121,122,123,124,
							200,201,202,203,205,207,208,209,210,212,213,214,
							215,217,219,220,221,222,223,228,230,231,232,233,234} ;

#else
#define ECG_DB_PATH	"C:\\AHADAT~1\\"	// Path to where AHA data.
#define REC_COUNT	69
int Records[REC_COUNT] = {1201,1202,1203,1204,1205,1206,1207,1208,1209,1210,
									2201,2203,2204,2205,2206,2207,2208,2209,2210,
									3201,3202,3203,3204,3205,3206,3207,3208,3209,3210,
									4201,4202,4203,4204,4205,4206,4207,4208,4209,4210,
									5201,5202,5203,5204,5205,5206,5207,5208,5209,5210,
									6201,6202,6203,6204,6205,6206,6207,6208,6209,6210,
									7201,7202,7203,7204,7205,7206,7207,7208,7209,7210} ;
#endif
// External function prototypes.
void ResetBDAC(void) ;
int BeatDetectAndClassify(int ecgSample, int *beatType, int *beatMatch) ;

// Local Prototypes.
int NextSample(int *vout,int nosig,int ifreq,
						int ofreq,int init) ;
int gcd(int x, int y);

// Global variables.

int ADCZero, ADCUnit, InputFileSampleFrequency ;

int main(void)
	{
		char record[10], fname[20] ;
		int i, ecg[2], delay, recNum ;
		WFDB_Siginfo s[2] ;
		WFDB_Anninfo a[2] ;
		WFDB_Annotation annot ;

		unsigned char byte ;
		FILE *newAnn0, *newAnn1 ;
		long SampleCount = 0, lTemp, DetectionTime ;
		int beatType, beatMatch ;
	
	//printf("Hello Open C!\n");
	//printf("Press a character to exit!");
	  printf("This is a demo for the ECG program!\n");
	//int c = getchar();
	setwfdb(ECG_DB_PATH) ;
	
	// Analyze all 48 MIT/BIH Records.
	 
	for(recNum = 0; recNum < REC_COUNT; ++recNum)
		{
		sprintf(record,"%d",Records[recNum]) ;
		printf("Reading...\n");
		printf("Record %d\n",Records[recNum]) ;
		
		 
		// Open a 2 channel record

		if(isigopen(record,s,2) < 1)
			{
			printf("Couldn't open %s\n",record) ;
			return 0;
			}
		
		ADCZero = s[0].adczero ;
		ADCUnit = s[0].gain ;
		InputFileSampleFrequency = sampfreq(record) ;

		// Setup for output annotations

		a[0].name = "atest"; a[0].stat = WFDB_WRITE ;

		if(annopen(record, a, 1) < 0)
			//int c = getchar();
			return 0;
		
		// Initialize sampling frequency adjustment.

		NextSample(ecg,2,InputFileSampleFrequency,SAMPLE_RATE,1) ;

		// Initialize beat detection and classification.

		ResetBDAC() ;
		SampleCount = 0 ;

		// Read data from MIT/BIH file until there is none left.

		while(NextSample(ecg,2,InputFileSampleFrequency,SAMPLE_RATE,0) >= 0)
			{
			++SampleCount ;

			// Set baseline to 0 and resolution to 5 mV/lsb (200 units/mV)

			lTemp = ecg[0]-ADCZero ;
			lTemp *= 200 ;
			lTemp /= ADCUnit ;
			ecg[0] = lTemp ;

			// Pass sample to beat detection and classification.

			delay = BeatDetectAndClassify(ecg[0], &beatType, &beatMatch) ;

			// If a beat was detected, annotate the beat location
			// and type.

			if(delay != 0)
				{
				DetectionTime = SampleCount - delay ;

				// Convert sample count to input file sample
				// rate.

				DetectionTime *= InputFileSampleFrequency ;
				DetectionTime /= SAMPLE_RATE ;
				annot.time = DetectionTime ;
				annot.anntyp = beatType ;
				annot.aux = NULL ;
				putann(0,&annot) ;
				}
			}

		// Reset database after record is done.

		wfdbquit() ;

		// Copy "atest.<record>" to "<record>.ate" for future ascess.
		// (This is necessary for PC files)

		//sprintf(fname,"%s.ate",record) ;
		sprintf(fname,"%s.atest",record) ;
		newAnn0 = fopen(fname,"rb") ;
		sprintf(fname,"%s%s.ate",ECG_DB_PATH,record) ;
		newAnn1 = fopen(fname,"wb") ;

		// Copy byte image of annotation file in this
		// directory to a correctly named file in the
		// database directory.

		while(fread(&byte,sizeof(char),1,newAnn0) == 1)
			fwrite(&byte,sizeof(char),1,newAnn1) ;

		fclose(newAnn0) ;
      fclose(newAnn1) ;
      
     
	
	}
	int c = getchar();
	return 0;
	}
	/**********************************************************************
		NextSample reads MIT/BIH Arrhythmia data from a file of data
		sampled at ifreq and returns data sampled at ofreq.  Data is
		returned in vout via *vout.  NextSample must be initialized by
		passing in a nonzero value in init.  NextSample returns -1 when
	   there is no more data left.
	***********************************************************************/

	int  NextSample(int *vout,int nosig,int ifreq,
							int ofreq,int init)
		{
		int i ;
		static int m, n, mn, ot, it, vv[WFDB_MAXSIG], v[WFDB_MAXSIG], rval ;

		if(init)
			{
			i = gcd(ifreq, ofreq);
			m = ifreq/i;
			n = ofreq/i;
			mn = m*n;
			ot = it = 0 ;
			getvec(vv) ;
			rval = getvec(v) ;
			}

		else
			{
			while(ot > it)
				{
		    	for(i = 0; i < nosig; ++i)
		    		vv[i] = v[i] ;
				rval = getvec(v) ;
			    if (it > mn) { it -= mn; ot -= mn; }
			    it += n;
			    }		
		    for(i = 0; i < nosig; ++i)
		    	vout[i] = vv[i] + (ot%n)*(v[i]-vv[i])/n;
			ot += m;
			}

		return(rval) ;
		}

	// Greatest common divisor of x and y (Euclid's algorithm)

	int gcd(int x, int y)
		{
		while (x != y) {
			if (x > y) x-=y;
			else y -= x;
			}
		return (x);
		}
	
