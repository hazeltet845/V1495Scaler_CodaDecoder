#include <iostream>
#include <iomanip>

#include <TList.h>
#include <TH1.h>
#include <TF1.h>
#include <TString.h>
#include <TH1D.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <vector>

#include "THaCodaFile.h"
#include "THaEtClient.h"

#define MAX_EVENT_SIZE 100000

using namespace std;



int main(int argc, char* argv[])
{
    THaCodaData* coda = new THaCodaFile(TString(argv[1]));



    //Book output tuple
    unsigned int event_ty;
    unsigned int event_sp_n; //spill event #
    unsigned int s_scaler_a[32];
    unsigned int s_scaler_b[32];
    unsigned int s_scaler_d[32];
    unsigned int s_scaler_e[32];
    unsigned int ch_num[128];
    // int f_scaler[];
    unsigned int spill_id;

    unsigned int nWordsTotal;

    TFile* saveFile = new TFile(argv[2], "recreate");
    TTree* SlowScalTree = new TTree("SlowScalTree", "SlowScalTree");
    TTree* FastScalTree = new TTree("FastScalTree", "FastScalTree");

/*
    saveTree->Branch("channel", channel, "channel[32]/I");
    saveTree->Branch("eventID", &eventID);
    saveTree->Branch("scaler", &scaler, "scaler[32]/I");
*/
    
    SlowScalTree->Branch("s_scaler_a", &s_scaler_a, "s_scaler_a[32]/I");
    SlowScalTree->Branch("s_scaler_b", &s_scaler_b, "s_scaler_b[32]/I");
    SlowScalTree->Branch("s_scaler_d", &s_scaler_d, "s_scaler_d[32]/I");
    SlowScalTree->Branch("s_scaler_e", &s_scaler_e, "s_scaler_e[32]/I");
   
    SlowScalTree->Branch("event_ty", &event_ty);
    SlowScalTree->Branch("event_sp_n", &event_sp_n);
    SlowScalTree->Branch("spill_id", &spill_id);
    SlowScalTree->Branch("ch_num", &ch_num, "ch_num[128]/I");
    
   
    unsigned int* data;
    spill_id = 0; 
    event_sp_n = 0;
    int cnt = 0;

    int cnt0 = 0; 

    while(true)
    {
      int status = coda->codaRead();
      if(status != 0)
      {
          if(status == -1)
          {
              coda->codaClose();
              break;
          }
          else
          {
              cout << "Spotted a corruptted event." << endl;
              continue;
          }
      }
 
      cnt0 = cnt0 + 1;

      data = coda->getEvBuffer();
      event_ty = data[1] >> 16;
      nWordsTotal = data[0] + 1;
     // int cnt = 0;
    //  printf("Event # %i, eventType = %i, nWords = %i \n", event, eventType, nWordsTotal);
     
      
      if(event_ty == 2){
         cnt = cnt + 1; 
         printf("Slow Scaler Event %i \n", cnt); 
           
        //map all the branch elements first   
        unsigned int n=0;
        while (n<nWordsTotal){
            if(data[n] == 0xaaaaaaaa){
                n = n + 4;
                for(int i = 0; i < 32; i++){
                    ch_num[i] = i;
                    s_scaler_a[i] = data[n+i];
                   // printf("data %i \n", s_scaler_a[i]);
                   // printf("ch_num %i \n", ch_num[i]);
                }    
                n = n + 32;
            }
            if(data[n] == 0xbbbbbbbb){
                n = n + 4;
                for(int i = 0; i < 32; i++){
                    ch_num[i+32] = i+32;
                    s_scaler_b[i] = data[n+i];

                }    
                n = n + 32;
            }
            if(data[n] == 0xdddddddd){
                n = n + 4;
                for(int i = 0; i < 32; i++){
                    ch_num[i+64] = i+64;
                    s_scaler_d[i] = data[n+i];

                }    
                n = n + 32;
            }
            if(data[n] == 0xeeeeeeee){
                n = n + 4;
                for(int i = 0; i < 32; i++){
                    ch_num[i+96] = i+96;
                    s_scaler_e[i] = data[n+i];

                }    
                n = n + 32;
            }
            
            n++;
        }
        
        event_sp_n = 0;
        //once every branch element is assingmed 
        //fill the tree
        SlowScalTree->Fill();

        spill_id++;
      } //ending of if statement with event type 2  

      if(event_ty == 1){
       
        event_sp_n++;
      } //ending of if statement with event type 1
 
    } //ending while loop over ALL CODA EVENTS

    printf("# of events: %i \n", cnt0);

    saveFile->cd();
    SlowScalTree->Write();
    FastScalTree->Write();

    saveFile->Close();

    coda->codaClose();
    return 0;
}
//===========================================================================================
