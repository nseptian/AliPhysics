
AliAnalysisTask *AddTask_acapon(TString outputFileName = "AnalysisResult.root",
                                TString names          = "kCutSet1",
                                Bool_t SDDstatus       = kFALSE,
                                Bool_t hasMC           = kFALSE,
                                Int_t wagonNum         = 0,
                                Bool_t doPairing       = kTRUE,
                                Bool_t applyPairCuts   = kTRUE,
                                Bool_t doMixing        = kTRUE, // Do event mixing for R fac.
                                Bool_t trackVarPlots   = kTRUE, // Simple track QA plots
                                Int_t whichDetPlots    = 0,     // 0=None,1=ITS,2=TPC,4=TOF,7=All3
                                Bool_t v0plots         = kTRUE, // Plots for PID cal.
                                Bool_t useITScorr      = kTRUE,
                                Bool_t useTPCcorr      = kTRUE,
                                Bool_t useTOFcorr      = kTRUE,
                                Bool_t plots3D         = kFALSE,
                                Bool_t useRun1binning  = kFALSE,
                                Bool_t getFromAlien    = kFALSE)
{
  
    TObjArray *arrNames = names.Tokenize(";");
    Int_t nDie = arrNames->GetEntries();
    Printf("Number of implemented cuts: %i", nDie);
    std::cout << "Output file name: " << outputFileName << std::endl;
    std::cout << "Cut set         : " << names          << std::endl;
    std::cout << "Use SDD         : " << SDDstatus      << std::endl;
    std::cout << "Monte Carlo     : " << hasMC          << std::endl;
    std::cout << "Wagon number    : " << wagonNum       << std::endl;
    std::cout << "Pairing         : " << doPairing      << std::endl;
    std::cout << "Pair cuts       : " << applyPairCuts  << std::endl;
    std::cout << "Event mixing    : " << doMixing       << std::endl;
    std::cout << "Track plots     : " << trackVarPlots  << std::endl;
    std::cout << "Which det plots : " << whichDetPlots  << std::endl;
    std::cout << "v0 plots        : " << v0plots        << std::endl;
    std::cout << "Use ITScorr     : " << useITScorr     << std::endl;
    std::cout << "Use TPCcorr     : " << useTPCcorr     << std::endl;
    std::cout << "Use TOFcorr     : " << useTOFcorr     << std::endl;
    std::cout << "3D plots        : " << plots3D        << std::endl;
    std::cout << "Using Run1 bins : " << useRun1binning << std::endl;

    //get the current analysis manager
    AliAnalysisManager *mgr = AliAnalysisManager::GetAnalysisManager();
    if(!mgr){
      Error("AddTask_acapon", "No analysis manager found.");
      return 0;
    }

    TString configBasePath("$ALICE_PHYSICS/PWGDQ/dielectron/macrosLMEE/");
    TString configFile("Config_acapon.C");
    TString configLMEECutLib("LMEECutLib_acapon.C");

    // Load updated macros from private ALIEN path
    TString myConfig ="alien_cp alien:///alice/cern.ch/user/a/acapon/PWGDQ/dielectron/macrosLMEE/Config_acapon.C .";
    TString myCutLib ="alien_cp alien:///alice/cern.ch/user/a/acapon/PWGDQ/dielectron/macrosLMEE/LMEECutLib_acapon.C ."; 
    if(getFromAlien && (!gSystem->Exec(myConfig)) && (!gSystem->Exec(myCutLib))){
      std::cout << "Copy config from Alien" << std::endl;
      configBasePath=Form("%s/",gSystem->pwd());
    }

    TString configFilePath(configBasePath+configFile);
    TString configLMEECutLibPath(configBasePath+configLMEECutLib);

    //load dielectron configuration files
    if(!gROOT->GetListOfGlobalFunctions()->FindObject(configLMEECutLib.Data())){
      gROOT->LoadMacro(configLMEECutLibPath.Data());
    }
    if(!gROOT->GetListOfGlobalFunctions()->FindObject(configFile.Data())){
      gROOT->LoadMacro(configFilePath.Data());
    }

    // Cut lib
    LMEECutLib* cutlib = new LMEECutLib(SDDstatus);

    // AOD Usage currently tested with Input handler
    if(mgr->GetInputEventHandler()->IsA()==AliAODInputHandler::Class()){
      ::Info("AddTask_acapon", "no dedicated AOD configuration"); 
    }
    else if(mgr->GetInputEventHandler()->IsA()==AliESDInputHandler::Class()){
      ::Info("AddTask_acapon","switching on ESD specific code");
      //bESDANA = kTRUE;
    }

    //create task and add it to the manager
    AliAnalysisTaskMultiDielectron* task = new AliAnalysisTaskMultiDielectron(Form("DielectronTask%d", wagonNum));
    if(!task){
      ::Error("AddTask_acapon", "MultiDielectron trask not created");
      return 0x0;
    }

    // Add event filter (the same for all cut sets)
    Int_t triggerNames = (AliVEvent::kINT7);
    task->SetEventFilter(cutlib->GetEventCuts());
    task->SelectCollisionCandidates(triggerNames);
    task->SetTriggerMask(triggerNames);
    //task->SetRejectPileup(); //Rejection applied via train dependancy

    // Add the task to the manager
    mgr->AddTask(task);
    // Add dielectron analysis with different cuts to the task
    for(Int_t i = 0; i < nDie; ++i){ 
      TString dielTaskName(arrNames->At(i)->GetName());
      AliDielectron* diel_low = Config_acapon(dielTaskName, hasMC, SDDstatus,
                                              doPairing, applyPairCuts, doMixing, 
                                              trackVarPlots, whichDetPlots, v0plots,
                                              useITScorr, useTPCcorr, useTOFcorr,
                                              plots3D, useRun1binning);
      if(!diel_low){
        continue;
      }
      task->AddDielectron(diel_low);
      printf("successfully added AliDielectron: %s\n",diel_low->GetName());
    }// End cut settings initialisation loop


    // Create output container
    AliAnalysisDataContainer *coutput1 =
    mgr->CreateContainer(Form("acapon_tree_%d",wagonNum),
                         TTree::Class(),
                         AliAnalysisManager::kExchangeContainer,
                         outputFileName.Data());

    AliAnalysisDataContainer *cOutputHist1 =
    mgr->CreateContainer(Form("acapon_out_%d", wagonNum),
                         TList::Class(),
                         AliAnalysisManager::kOutputContainer,
                         outputFileName.Data());

    AliAnalysisDataContainer *cOutputHist2 =
    mgr->CreateContainer(Form("acapon_CF_%d", wagonNum),
                         TList::Class(),
                         AliAnalysisManager::kOutputContainer,
                         outputFileName.Data());

    AliAnalysisDataContainer *cOutputHist3 =
    mgr->CreateContainer(Form("acapon_EventStat_%d", wagonNum),
                         TH1D::Class(),
                         AliAnalysisManager::kOutputContainer,
                         outputFileName.Data());


    mgr->ConnectInput(task,  0, mgr->GetCommonInputContainer());
    mgr->ConnectOutput(task, 0, coutput1 );
    mgr->ConnectOutput(task, 1, cOutputHist1);
    mgr->ConnectOutput(task, 2, cOutputHist2);
    mgr->ConnectOutput(task, 3, cOutputHist3);

    return task;
}
