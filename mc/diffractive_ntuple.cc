//STANDARD ROOT INCLUDES
#include <TROOT.h>
#include <TF1.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TTree.h>
#include <TFile.h>
#include <TChain.h>
#include <TChainElement.h>
#include <TDirectory.h>
#include <TSystemFile.h>
#include <TSystemDirectory.h>
#include <TRandom.h>

//STANDARD C++ INCLUDES
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>

//OUR OWN CLASSES TO READ THE TREE
#include "MassParticles.h"
#include "MyBaseJet.h"
#include "MyBeamSpot.h"
#include "MyCaloJet.h"
#include "MyCastorDigi.h"
#include "MyCastorJet.h"
#include "MyCastorRecHit.h"
#include "MyDiJet.h"
#include "MyElectron.h"
#include "MyEvtId.h"
#include "MyFwdGap.h"
#include "MyGenJet.h"
#include "MyGenKin.h"
#include "MyGenMet.h"
#include "MyGenPart.h"
#include "MyHLTrig.h"
#include "MyJet.h"
#include "MyL1Trig.h"
#include "MyL1TrigOld.h"
//#include "MyMITEvtSel.h"
#include "MyMet.h"
#include "MyMuon.h"
#include "MyPFCand.h"
#include "MyCaloTower.h"
#include "MyCaloJet.h"
#include "MyPFJet.h"
#include "MyPUSumInfo.h"
#include "MyPart.h"
#include "MySimVertex.h"
#include "MyTracks.h"
#include "MyVertex.h"
#include "RPRootDumpReconstructedProton.h"
#include "RPRootDumpTrackInfo.h"

#include "rp_aperture_config.h"
#include "analysis_tools.h"
#include "beam_vtx_smearing.h"

//#include "CondFormats/JetMETObjects/interface/FactorizedJetCorrector.h"
//#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
//#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
//#include "JetCorrectorParameters.h"
//#include "JetCorrectionUncertainty.h"
//#include "FactorizedJetCorrector.h"

//ROOUNFOLD CLASSES
//#include "/storage/lhuertas/uerj-1/CMSTOTEM/mc/RooUnfold-1.1.1/src/RooUnfoldResponse.h"
//#include "/storage/lhuertas/uerj-1/CMSTOTEM/mc/RooUnfold-1.1.1/src/RooUnfoldBayes.h"
//#include "/storage/lhuertas/uerj-1/CMSTOTEM/mc/RooUnfold-1.1.1/src/RooUnfoldBinByBin.h"

#define PI 3.141592653589793
#define M_P 0.938272046
using namespace std;

bool sortByPt(MyBaseJet const& jet1, MyBaseJet const& jet2){
   return ( jet1.Pt() > jet2.Pt() );
}

//JetCorrectorParameters *L2Relative, *L3Absolute, *L2L3Residual;
//vector<JetCorrectorParameters> vecL2Relative, vecL3Absolute, vecL2L3Residual;

void diffractive_ntuple(string const& mc = "pomwig", bool reggeon = true, bool side_minus = true, bool side_plus = false, bool unfold = false, const Int_t nevt_max = -1){
  
  TString file_name, side;
  if (side_minus && !side_plus) side = "minus";
  if (!side_minus && side_plus) side = "plus";
  if (side_minus && side_plus) side = "bothsides";
  TString regg = (reggeon) ? "_reggeon" : ""; 
  file_name = mc + regg + "_" + side + "_ntuple.root";
  TString outputFileName = "/storage/lhuertas/uerj-1/CMSTOTEM/mc/Workspace/root_files/" + file_name; 
  cout<<outputFileName<<endl;


  bool verbose = false;
  string treeName = "evt";//"cms_totem";
  string jetCollName = "ak5PFJets";
//  string jetCorrName = "ak5PFL2L3Residual";
  string jetCorrName = "ak5PFL2L3"; 
  //string jetCorrName = "ak5PFJets"; 

  ThresholdsPerRegion thresholdsPFlow;
  thresholdsPFlow[Barrel] = ThresholdsPerType(); 
  thresholdsPFlow[Endcap] = ThresholdsPerType(); 
  thresholdsPFlow[Transition] = ThresholdsPerType(); 
  thresholdsPFlow[Endcap] = ThresholdsPerType(); 
  resetPFThresholds(thresholdsPFlow[Barrel]);
  resetPFThresholds(thresholdsPFlow[Endcap]);
  resetPFThresholds(thresholdsPFlow[Transition]);
  resetPFThresholds(thresholdsPFlow[Forward]);

  thresholdsPFlow[Barrel][MyPFCand::h0]            = make_pair(-1.,1.4);
  thresholdsPFlow[Barrel][MyPFCand::gamma]         = make_pair(-1.,0.9);
  thresholdsPFlow[Endcap][MyPFCand::h0]            = make_pair(-1.,2.7);
  thresholdsPFlow[Endcap][MyPFCand::gamma]         = make_pair(-1.,2.5);
  thresholdsPFlow[Transition][MyPFCand::h0]        = make_pair(-1.,3.8);
  thresholdsPFlow[Transition][MyPFCand::gamma]     = make_pair(-1.,2.5);
  thresholdsPFlow[Transition][MyPFCand::h_HF]      = make_pair(-1.,4.0);
  thresholdsPFlow[Transition][MyPFCand::egamma_HF] = make_pair(-1.,3.5);
  thresholdsPFlow[Forward][MyPFCand::h_HF]         = make_pair(-1.,4.0);
  thresholdsPFlow[Forward][MyPFCand::egamma_HF]    = make_pair(-1.,3.5);

  ThresholdsPerType::const_iterator pfThreshold = thresholdsPFlow[Barrel].begin();
  ThresholdsPerType::const_iterator pfThresholds_end = thresholdsPFlow[Barrel].end(); 
  ostringstream oss;
  oss << "Using the following PF thresholds:\n";
  for(; pfThreshold != pfThresholds_end; ++pfThreshold){
     int key = pfThreshold->first;    
     oss << "  " << key << ": "
                 << "(" << thresholdsPFlow[Barrel][key].first
                 << "," << thresholdsPFlow[Barrel][key].second << ")  "
                 << "(" << thresholdsPFlow[Endcap][key].first
                 << "," << thresholdsPFlow[Endcap][key].second << ")  "
                 << "(" << thresholdsPFlow[Transition][key].first
                 << "," << thresholdsPFlow[Transition][key].second << ")  "
                 << "(" << thresholdsPFlow[Forward][key].first
                 << "," << thresholdsPFlow[Forward][key].second << ")\n";   
  }
  cout << oss.str();

  TH1F* p = new TH1F("","",100,2000,4000); 
  TH1F* theta = new TH1F("","",100,-0.001, 0.001);
  const Int_t nevt_max_corr = (nevt_max >= 0) ? nevt_max : 99999999;

  bool rp_right, rp_left, rp_right_accep_top, rp_right_accep_bottom, rp_left_accep_top, rp_left_accep_bottom;
  double xi_rec_cms_right, xi_rec_cms_left, xi_gen_cms_right, xi_gen_cms_left, xi_rec_proton_right, xi_rec_proton_left, xi_gen_proton_right, xi_gen_proton_left;
  double t_rec_proton_right, t_rec_proton_left, t_gen_proton_right, t_gen_proton_left, x_rec_right, x_rec_left, x_gen_right, x_gen_left;
  double t_rec_proton_right_gauss, t_rec_proton_left_gauss, t_gen_proton_right_gauss, t_gen_proton_left_gauss, xi_rec_proton_right_gauss, xi_rec_proton_left_gauss, xi_gen_proton_right_gauss, xi_gen_proton_left_gauss;
  double beta_rec_right, beta_rec_left, beta_gen_right, beta_gen_left, weight_mc;
  double jet1_rec_pt, jet1_rec_eta, jet1_rec_phi, jet2_rec_pt, jet2_rec_eta, jet2_rec_phi;
  double jet1_gen_pt, jet1_gen_eta, jet1_gen_phi, jet2_gen_pt, jet2_gen_eta, jet2_gen_phi;
  double theta_x_plus, theta_y_plus, theta_x_minus, theta_y_minus, rp_xpos_24, rp_xpos_124, rp_ypos_24, rp_ypos_124, rp_xpos_25, rp_xpos_125, rp_ypos_25, rp_ypos_125;
  double theta_x_plus_smear, theta_y_plus_smear, theta_x_minus_smear, theta_y_minus_smear;
  double px_proton_right_smear, px_proton_left_smear, py_proton_right_smear, py_proton_left_smear, pz_proton_right_smear, pz_proton_left_smear, e_proton_right_smear,e_proton_left_smear;
  double px_proton_right, px_proton_left, py_proton_right, py_proton_left, pz_proton_right, pz_proton_left, e_proton_right,e_proton_left, mass_proton_right, mass_proton_left;
  TTree* small_tree = new TTree("small_tree","");
  small_tree->Branch("weight_mc",&weight_mc,"weight_mc/D");
  small_tree->Branch("rp_xpos_24",&rp_xpos_24,"rp_xpos_24/D");
  small_tree->Branch("rp_ypos_24",&rp_ypos_24,"rp_ypos_24/D");
  small_tree->Branch("rp_xpos_124",&rp_xpos_124,"rp_xpos_124/D");
  small_tree->Branch("rp_ypos_124",&rp_ypos_124,"rp_ypos_124/D");
  small_tree->Branch("rp_xpos_25",&rp_xpos_25,"rp_xpos_25/D");
  small_tree->Branch("rp_ypos_25",&rp_ypos_25,"rp_ypos_25/D");
  small_tree->Branch("rp_xpos_125",&rp_xpos_125,"rp_xpos_125/D");
  small_tree->Branch("rp_ypos_125",&rp_ypos_125,"rp_ypos_125/D");
  small_tree->Branch("rp_right",&rp_right,"rp_right/O");
  small_tree->Branch("rp_right_accep_top",&rp_right_accep_top,"rp_right_accep_top/O");
  small_tree->Branch("rp_right_accep_bottom",&rp_right_accep_bottom,"rp_right_accep_bottom/O");
  small_tree->Branch("rp_left",&rp_left,"rp_left/O");
  small_tree->Branch("rp_left_accep_top",&rp_left_accep_top,"rp_left_accep_top/O");
  small_tree->Branch("rp_left_accep_bottom",&rp_left_accep_bottom,"rp_left_accep_bottom/O");
  small_tree->Branch("xi_rec_cms_right",&xi_rec_cms_right,"xi_rec_cms_right/D");
  small_tree->Branch("xi_gen_cms_right",&xi_gen_cms_right,"xi_gen_cms_right/D");
  small_tree->Branch("x_rec_right",&x_rec_right,"x_rec_right/D");
  small_tree->Branch("x_gen_right",&x_gen_right,"x_gen_right/D");
  small_tree->Branch("x_rec_left",&x_rec_left,"x_rec_left/D");
  small_tree->Branch("x_gen_left",&x_gen_left,"x_gen_left/D");
  small_tree->Branch("xi_rec_cms_left",&xi_rec_cms_left,"xi_rec_cms_left/D");
  small_tree->Branch("xi_gen_cms_left",&xi_gen_cms_left,"xi_gen_cms_left/D");
  small_tree->Branch("xi_rec_proton_right",&xi_rec_proton_right,"xi_rec_proton_right/D");
  small_tree->Branch("xi_gen_proton_right",&xi_gen_proton_right,"xi_gen_proton_right/D");
  small_tree->Branch("xi_rec_proton_left",&xi_rec_proton_left,"xi_rec_proton_left/D");
  small_tree->Branch("xi_gen_proton_left",&xi_gen_proton_left,"xi_gen_proton_left/D");
  small_tree->Branch("t_rec_proton_right",&t_rec_proton_right,"t_rec_proton_right/D");
  small_tree->Branch("t_gen_proton_right",&t_gen_proton_right,"t_gen_proton_right/D");
  small_tree->Branch("t_rec_proton_left",&t_rec_proton_left,"t_rec_proton_left/D");
  small_tree->Branch("t_gen_proton_left",&t_gen_proton_left,"t_gen_proton_left/D");
  small_tree->Branch("xi_rec_proton_right_gauss",&xi_rec_proton_right_gauss,"xi_rec_proton_right_gauss/D");
  small_tree->Branch("xi_gen_proton_right_gauss",&xi_gen_proton_right_gauss,"xi_gen_proton_right_gauss/D");
  small_tree->Branch("xi_rec_proton_left_gauss",&xi_rec_proton_left_gauss,"xi_rec_proton_left_gauss/D");
  small_tree->Branch("xi_gen_proton_left_gauss",&xi_gen_proton_left_gauss,"xi_gen_proton_left_gauss/D");
  small_tree->Branch("t_rec_proton_right_gauss",&t_rec_proton_right_gauss,"t_rec_proton_right_gauss/D");
  small_tree->Branch("t_gen_proton_right_gauss",&t_gen_proton_right_gauss,"t_gen_proton_right_gauss/D");
  small_tree->Branch("t_rec_proton_left_gauss",&t_rec_proton_left_gauss,"t_rec_proton_left_gauss/D");
  small_tree->Branch("t_gen_proton_left_gauss",&t_gen_proton_left_gauss,"t_gen_proton_left_gauss/D");
  small_tree->Branch("beta_rec_right",&beta_rec_right,"beta_rec_right/D");
  small_tree->Branch("beta_gen_right",&beta_gen_right,"beta_gen_right/D");
  small_tree->Branch("beta_rec_left",&beta_rec_left,"beta_rec_left/D");
  small_tree->Branch("beta_gen_left",&beta_gen_left,"beta_gen_left/D");
  small_tree->Branch("jet1_rec_pt",&jet1_rec_pt,"jet1_rec_pt/D");
  small_tree->Branch("jet1_rec_eta",&jet1_rec_eta,"jet1_rec_eta/D");
  small_tree->Branch("jet1_rec_phi",&jet1_rec_phi,"jet1_rec_phi/D");
  small_tree->Branch("jet1_gen_pt",&jet1_gen_pt,"jet1_gen_pt/D");
  small_tree->Branch("jet1_gen_eta",&jet1_gen_eta,"jet1_gen_eta/D");
  small_tree->Branch("jet1_gen_phi",&jet1_gen_phi,"jet1_gen_phi/D");
  small_tree->Branch("jet2_rec_pt",&jet2_rec_pt,"jet2_rec_pt/D");
  small_tree->Branch("jet2_rec_eta",&jet2_rec_eta,"jet2_rec_eta/D");
  small_tree->Branch("jet2_rec_phi",&jet2_rec_phi,"jet2_rec_phi/D");
  small_tree->Branch("jet2_gen_pt",&jet2_gen_pt,"jet2_gen_pt/D");
  small_tree->Branch("jet2_gen_eta",&jet2_gen_eta,"jet2_gen_eta/D");
  small_tree->Branch("jet2_gen_phi",&jet2_gen_phi,"jet2_gen_phi/D");
  small_tree->Branch("theta_x_plus",&theta_x_plus,"theta_x_plus/D");
  small_tree->Branch("theta_y_plus",&theta_y_plus,"theta_y_plus/D");
  small_tree->Branch("theta_x_minus",&theta_x_minus,"theta_x_minus/D");
  small_tree->Branch("theta_y_minus",&theta_y_minus,"theta_y_minus/D");
  //small_tree->Branch("theta_x_plus_smear",&theta_x_plus_smear,"theta_x_plus_smear/D");
  //small_tree->Branch("theta_y_plus_smear",&theta_y_plus_smear,"theta_y_plus_smear/D");
  //small_tree->Branch("theta_x_minus_smear",&theta_x_minus_smear,"theta_x_minus_smear/D");
  //small_tree->Branch("theta_y_minus_smear",&theta_y_minus_smear,"theta_y_minus_smear/D");
  small_tree->Branch("px_proton_right",&px_proton_right,"px_proton_right/D");
  small_tree->Branch("px_proton_left",&px_proton_left,"px_proton_left/D");
  small_tree->Branch("py_proton_right",&py_proton_right,"py_proton_right/D");
  small_tree->Branch("py_proton_left",&py_proton_left,"py_proton_left/D");
  small_tree->Branch("pz_proton_right",&pz_proton_right,"pz_proton_right/D");
  small_tree->Branch("pz_proton_left",&pz_proton_left,"pz_proton_left/D");
  //small_tree->Branch("px_proton_right_smear",&px_proton_right_smear,"px_proton_right_smear/D");
  //small_tree->Branch("px_proton_left_smear",&px_proton_left_smear,"px_proton_left_smear/D");
  //small_tree->Branch("py_proton_right_smear",&py_proton_right_smear,"py_proton_right_smear/D");
  //small_tree->Branch("py_proton_left_smear",&py_proton_left_smear,"py_proton_left_smear/D");
  //small_tree->Branch("pz_proton_right_smear",&pz_proton_right_smear,"pz_proton_right_smear/D");
  //small_tree->Branch("pz_proton_left_smear",&pz_proton_left_smear,"pz_proton_left_smear/D");
  small_tree->Branch("e_proton_right",&e_proton_right,"e_proton_right/D");
  small_tree->Branch("e_proton_left",&e_proton_left,"e_proton_left/D");
  //small_tree->Branch("e_proton_right_smear",&e_proton_right_smear,"e_proton_right_smear/D");
  //small_tree->Branch("e_proton_left_smear",&e_proton_left_smear,"e_proton_left_smear/D");
  small_tree->Branch("mass_proton_left",&mass_proton_left,"mass_proton_left/D");
  small_tree->Branch("mass_proton_right",&mass_proton_right,"mass_proton_right/D");

  small_tree->SetDirectory(0);

  map<string,TH2F*> histosTH2F;
  histosTH2F["pos_y_vs_x_proton_plus_024_025"] = new TH2F("pos_y_vs_x_proton_plus_024_025", "pos_y_vs_x_proton_plus" , 400, -0.05, 0.05, 400, -0.05, 0.05);
  histosTH2F["pos_y_vs_x_proton_plus_024_025_accept"] = new TH2F("pos_y_vs_x_proton_plus_024_025_accept", "pos_y_vs_x_proton_plus" , 400, -0.05, 0.05, 400, -0.05, 0.05);
  histosTH2F["pos_y_vs_x_proton_plus_024_025_tbin2"] = new TH2F("pos_y_vs_x_proton_plus_024_025_tbin2", "pos_y_vs_x_proton_plus" , 400, -0.05, 0.05, 400, -0.05, 0.05);
  histosTH2F["pos_y_vs_x_proton_minus_124_125"] = new TH2F("pos_y_vs_x_proton_minus_124_125", "pos_y_vs_x_proton_minus" , 400, -0.05, 0.05, 400, -0.05, 0.05);
  histosTH2F["pos_y_vs_x_proton_minus_124_125_accept"] = new TH2F("pos_y_vs_x_proton_minus_124_125_accept", "pos_y_vs_x_proton_minus" , 400, -0.05, 0.05, 400, -0.05, 0.05);
  histosTH2F["pos_y_vs_x_proton_minus_124_125_tbin2"] = new TH2F("pos_y_vs_x_proton_minus_124_125_tbin2", "pos_y_vs_x_proton_minus" , 400, -0.05, 0.05, 400, -0.05, 0.05);
  //===================
  int i_tot = 0 , nevt_tot = 0;
  
  // MC files
  const char *ext=".root";
  vector<TString>* vdirs = new vector<TString>;
  if (mc == "pomwig"){
     if (side_minus){ 
        if (!reggeon) vdirs->push_back("/storage/lhuertas/uerj-1/CMSTOTEM/samples/mc/Pomwig_SDDijetsMinus_8TeV/");
        else vdirs->push_back("/storage/lhuertas/uerj-1/CMSTOTEM/samples/mc/Pomwig_Reggeon_SDDijetsMinus_8TeV/");
     }
     if (side_plus){
        if (!reggeon) vdirs->push_back("/storage/lhuertas/uerj-1/CMSTOTEM/samples/mc/Pomwig_plus_test/");//SDDijetsPlus_8TeV/");
        else vdirs->push_back("/storage/lhuertas/uerj-1/CMSTOTEM/samples/mc/Pomwig_Reggeon_SDDijetsPlus_8TeV/");//SDDijetsPlus_8TeV/");
     }
  }
  if (mc == "pythia8_diff") vdirs->push_back("/storage/lhuertas/uerj-1/CMSTOTEM/samples/mc/Pythia8_SD_DD_Dijets_8Tev_Pt20/");

  vector<TString>* vfiles = new vector<TString>;
  for(vector<TString>::iterator itdirs = vdirs->begin(); itdirs != vdirs->end(); ++itdirs){
      TString& dirname = *itdirs;
      TSystemDirectory dir(dirname, dirname);
      TList *files = dir.GetListOfFiles();
      if (files) {
         TSystemFile *file;
         TString fname;
         TIter next(files);
         while ((file=(TSystemFile*)next())) {
             fname = file->GetName();
             if (!file->IsDirectory() && fname.EndsWith(ext)) {
                TString root_file = dirname + string(fname.Data());
                vfiles->push_back(root_file); cout<<root_file<<endl;      
             }
         }
       }
  }
  delete vdirs;

  //Declaration of tree and its branches variables
  TTree* tree = new TTree(treeName.c_str(),"");
  MyEvtId*           evtId        = NULL;
//   MyL1TrigOld*       l1Trig       = NULL;  
//   MyHLTrig*          hltTrig      = NULL;
  vector<MyGenPart>* genPart      = NULL;
  vector<MyTracks>*  track_coll   = NULL;
  vector<MyVertex>*  vertex_coll  = NULL;
  vector<MyCaloJet>*   caloJet_coll   = NULL;
  vector<MyPFJet>*   pfJet_coll   = NULL;
  vector<MyGenJet>*   genJet_coll   = NULL;
  vector<MyPFCand>*  pFlow_coll   = NULL;
  vector<MyCaloTower>*  caloTowers_coll   = NULL;
  MyGenKin*  genKin   = NULL;
  //=================================================

  // Jet energy corrections
  /*L2Relative = new JetCorrectorParameters("Winter14_V8/Winter14_V8_DATA_L2Relative_AK5PF.txt");
  L3Absolute = new JetCorrectorParameters("Winter14_V8/Winter14_V8_DATA_L3Absolute_AK5PF.txt");
  L2L3Residual = new JetCorrectorParameters("Winter14_V8/Winter14_V8_DATA_L2L3Residual_AK5PF.txt");
  vecL2Relative.push_back(*L2Relative);
  vecL3Absolute.push_back(*L3Absolute);
  vecL2L3Residual.push_back(*L2L3Residual);
  FactorizedJetCorrector *jecL2Relative   = new FactorizedJetCorrector(vecL2Relative);
  FactorizedJetCorrector *jecL3Absolute   = new FactorizedJetCorrector(vecL3Absolute);
  FactorizedJetCorrector *jecL2L3Residual = new FactorizedJetCorrector(vecL2L3Residual);
*/
  rp_aperture_config();
  
  double nweight_total = 0;
  double nevents_total = 0; 
  //starting Loop over files, stops at end of list of files or when reached nevt_max
  for(vector<TString>::iterator itfiles = vfiles->begin(); itfiles != vfiles->end() && i_tot < nevt_max_corr; ++itfiles){
  
    TFile* file = TFile::Open(*itfiles,"READ");
    
    //getting the tree form the current file
    tree = (TTree*) file->Get( treeName.c_str() );

    //Getting number of events
    int nev = int(tree->GetEntriesFast());
    nevt_tot += nev;
    cout <<"The current file has " << nev << " entries : "<< endl << *itfiles << endl;

    //adding branches to the tree ----------------------------------------------------------------------
    tree->SetBranchAddress("evtId",&evtId);
    tree->SetBranchAddress("generalTracks",&track_coll); 
    tree->SetBranchAddress("offlinePrimaryVertices",&vertex_coll);
    tree->SetBranchAddress("ak5CaloJets",&caloJet_coll);
    tree->SetBranchAddress("ak5PFJets",&pfJet_coll);
    tree->SetBranchAddress("ak5GenJets",&genJet_coll);
    tree->SetBranchAddress("particleFlow",&pFlow_coll);
    tree->SetBranchAddress("genKin",&genKin);
    tree->SetBranchAddress("genPart",&genPart);
    tree->SetBranchAddress("caloTowers",&caloTowers_coll);
  
    /*//Getting number of events
    int nev = int(tree->GetEntriesFast());
    nevt_tot += nev;
    cout <<"The current file has " << nev << " entries : "<< endl << *itfiles << endl;*/

     weight_mc = -1.; 
     //xi_cms_st = -999.; xi_totem_st = -999.; xi_totem_sel=-999.; xi_cms_minus_totem_st = -999.;
  
    //starting loop over events, stops when reached end of file or nevt_max
    for(int i_evt = 0; i_evt < nev && i_tot < nevt_max_corr; ++i_evt , ++i_tot){
    
    //printing the % of events done every 10k evts
    if( ((i_tot+1) % 10000) == 0) cout <<int(double(i_tot+1)/1000)<<"k done"<<endl;
    
      //Filling the variables defined setting branches
      tree->GetEntry(i_evt);

      bool passedHLT = false;
      bool passedvtx = false;
      bool jet1_rec_selected = false;
      bool jet2_rec_selected = false;
      bool jet1_gen_selected = false;
      bool jet2_gen_selected = false;
      bool jet1_pt20_rec_selected = false;
      bool jet2_pt20_rec_selected = false;
      bool jet1_pt20_gen_selected = false;
      bool jet2_pt20_gen_selected = false;
      bool pz_proton_max = false;
      bool PF_eta_max = false;
      bool PF_eta_min = false;
      bool xi_negat_gen = false;
      bool xi_posit_gen = false;
      
      //AT THIS POINT ON, CAN START USING THE VARIABLES LIKE TRACKS, VERTEX ETC !
      double weight = genKin->genWeight; 
      double event_weight = (mc == "pomwig") ? 1.0 : weight;
      nweight_total += weight; 
      ++nevents_total;
  
      bool sd_minus_pythia = false;
      bool sd_plus_pythia = false;
      bool dd_pythia = false;

      if (mc == "pythia8_diff"){
         int process_id = genKin->MCProcId;
         if (process_id == 103) sd_minus_pythia = true;
         if (process_id == 104) sd_plus_pythia = true;
         if (process_id == 105) dd_pythia = true;
      }
      if (dd_pythia) continue;
 
 	    
     // Vertices
      MyVertex& primaryVertex = vertex_coll->at(0);
      bool select_Vertex = ( !primaryVertex.fake && primaryVertex.validity && primaryVertex.ndof > 4);// && fabs( primaryVertex.z ) < 15.0 && prim_vtx_r < 2.0);
      if (!select_Vertex) continue;


      //Jets with pt>30Gev and !eta!<2
      Double_t Jet1_pt_rec; 
      Double_t Jet2_pt_rec; 
      Double_t Jet1_eta_rec; 
      Double_t Jet2_eta_rec; 
      Double_t Jet1_phi_rec, Jet1_px_rec, Jet1_py_rec, Jet1_pz_rec, Jet1_energy_rec; 
      Double_t Jet2_phi_rec, Jet2_px_rec, Jet2_py_rec, Jet2_pz_rec, Jet2_energy_rec;

      vector<MyBaseJet> JetVectorCorrected;
      JetVectorCorrected.resize( pfJet_coll->size() );
      size_t idx_jet = 0;
      for(vector<MyPFJet>::iterator it_jet = pfJet_coll->begin(); it_jet != pfJet_coll->end() ; ++it_jet,++idx_jet){
         map<string,MyBaseJet>::iterator it_map = it_jet->mapjet.begin();
         for(; it_map != it_jet->mapjet.end(); ++it_map)
            if(verbose) cout << it_map->first << endl;

         MyBaseJet const& basejet = it_jet->mapjet[jetCorrName];
/*
         TLorentzVector oldJet;
         oldJet.SetPxPyPzE(basejet.Px(), basejet.Py(), basejet.Pz(), basejet.E());
         TLorentzVector UnCorrectedJet = oldJet*(1/basejet.jec);
         //cout<<Jet1_px<<"  "<<UnCorrectedJet1.Px()<<endl;   
         // ---- Evaluating the L2Relative correction factor ---- //
         jecL2Relative->setJetPt(UnCorrectedJet.Pt());
         jecL2Relative->setJetEta(UnCorrectedJet.Eta());
         double corFactorL2Relative = jecL2Relative->getCorrection();
               //cout<<"L2Relative Cor Factor"<<corFactorL2Relative<<endl;
         TLorentzVector JetL2Relative = UnCorrectedJet*corFactorL2Relative;

         // ---- Evaluating the L3Absolute correction factor ---- //
         jecL3Absolute->setJetPt(JetL2Relative.Pt());
         jecL3Absolute->setJetEta(JetL2Relative.Eta());
         double corFactorL3Absolute = jecL3Absolute->getCorrection();
         TLorentzVector JetL2RelativeL3Absolute = JetL2Relative*corFactorL3Absolute;

         // ---- Evaluating the L2L3Rsidual correction factor ---- //
         jecL2L3Residual->setJetPt(JetL2RelativeL3Absolute.Pt());
         jecL2L3Residual->setJetEta(JetL2RelativeL3Absolute.Eta());
         double corFactorL2L3Residual(-1.);
         corFactorL2L3Residual = jecL2L3Residual->getCorrection();
         TLorentzVector JetL2RelativeL3AbsoluteL2L3Residual = JetL2RelativeL3Absolute*corFactorL2L3Residual;

         double CoorFactor = JetL2RelativeL3AbsoluteL2L3Residual.Pt()/UnCorrectedJet.Pt();

         JetVectorCorrected[idx_jet].SetPxPyPzE( JetL2RelativeL3AbsoluteL2L3Residual.Px(),
                                                 JetL2RelativeL3AbsoluteL2L3Residual.Py(),
                                                 JetL2RelativeL3AbsoluteL2L3Residual.Pz(),
                                                 JetL2RelativeL3AbsoluteL2L3Residual.E() );
         JetVectorCorrected[idx_jet].jec = CoorFactor;
  */    }

    //  std::stable_sort(JetVectorCorrected.begin(),JetVectorCorrected.end(),sortByPt);

      
      if( pfJet_coll->size() > 0 ){
	 MyBaseJet const& leadingJet = ( pfJet_coll->at(0) ).mapjet[jetCorrName];
	 //MyBaseJet const& leadingJet = ( JetVectorCorrected.at(0) );
	 Jet1_pt_rec = leadingJet.Pt(); 
	 Jet1_eta_rec = leadingJet.Eta(); 
	 Jet1_phi_rec = leadingJet.Phi(); 
	 Jet1_px_rec = leadingJet.Px(); 
	 Jet1_py_rec = leadingJet.Py(); 
	 Jet1_pz_rec = leadingJet.Pz(); 
	 Jet1_energy_rec = leadingJet.E(); 
      }
      //if(!jet1_rec_selected) continue;
      
      if( pfJet_coll->size() > 1 ){
	 MyBaseJet const& secondJet = ( pfJet_coll->at(1) ).mapjet[jetCorrName];
	 //MyBaseJet const& secondJet = ( JetVectorCorrected.at(1) );
         Jet2_pt_rec = secondJet.Pt(); 
	 Jet2_eta_rec = secondJet.Eta(); 
	 Jet2_phi_rec = secondJet.Phi(); 
         Jet2_px_rec = secondJet.Px();
         Jet2_py_rec = secondJet.Py();
         Jet2_pz_rec = secondJet.Pz();
         Jet2_energy_rec = secondJet.E();
      }
      //if(!jet2_rec_selected) continue;
      double mass_jets_rec= sqrt(pow(Jet1_energy_rec+Jet2_energy_rec,2)-pow(Jet1_px_rec+Jet2_px_rec,2)-pow(Jet1_py_rec+Jet2_py_rec,2)-pow(Jet1_pz_rec+Jet2_pz_rec,2));
      double x_minus_rec = ((Jet1_energy_rec-Jet1_pz_rec)+(Jet2_energy_rec-Jet2_pz_rec))/8000; 
      double x_plus_rec = ((Jet1_energy_rec+Jet1_pz_rec)+(Jet2_energy_rec+Jet2_pz_rec))/8000; 



      //Jet generated level         
      double leadingJet_pt_gen = -999;
      double secondJet_pt_gen = -999;
      double Jet1_energy_gen;
      double Jet1_px_gen;
      double Jet1_py_gen;
      double Jet1_pz_gen;
      double Jet2_energy_gen;
      double Jet2_px_gen;
      double Jet2_py_gen;
      double Jet2_pz_gen;
      double Jet1_eta_gen;
      double Jet1_phi_gen;
      double Jet2_eta_gen;
      double Jet2_phi_gen;


      for(vector<MyGenJet>::iterator it_genjet = genJet_coll->begin(); it_genjet != genJet_coll->end(); ++it_genjet){
         double jet_pt_gen = it_genjet->Pt();
         double jet_eta_gen = it_genjet->Eta();
         double jet_ene_gen = it_genjet->E();
         double jet_px_gen = it_genjet->Px();
         double jet_py_gen = it_genjet->Py();
         double jet_pz_gen = it_genjet->Pz();
         double jet_phi_gen = it_genjet->Phi();

        // if (fabs(jet_eta_gen)>4.4) continue;

         if (jet_pt_gen>leadingJet_pt_gen){
             leadingJet_pt_gen = jet_pt_gen;
             Jet1_energy_gen = jet_ene_gen;
             Jet1_px_gen = jet_px_gen;
             Jet1_py_gen = jet_py_gen;
             Jet1_pz_gen = jet_pz_gen;
             Jet1_eta_gen = jet_eta_gen;
             Jet1_phi_gen = jet_phi_gen;
         }
         if (jet_pt_gen>secondJet_pt_gen && jet_pt_gen<leadingJet_pt_gen){
             secondJet_pt_gen = jet_pt_gen;
             Jet2_energy_gen = jet_ene_gen;
             Jet2_px_gen = jet_px_gen;
             Jet2_py_gen = jet_py_gen;
             Jet2_pz_gen = jet_pz_gen;
             Jet2_eta_gen = jet_eta_gen;
             Jet2_phi_gen = jet_phi_gen;
         }
      }
      if(leadingJet_pt_gen>30. && fabs(Jet1_eta_gen)<4.4) jet1_gen_selected = true;
      if(secondJet_pt_gen>30. && fabs(Jet2_eta_gen)<4.4) jet2_gen_selected = true;
      if(leadingJet_pt_gen>20. && fabs(Jet1_eta_gen)<4.4) jet1_pt20_gen_selected = true;
      if(secondJet_pt_gen>20. && fabs(Jet2_eta_gen)<4.4) jet2_pt20_gen_selected = true;

      double mass_jets_gen= sqrt(pow(Jet1_energy_gen+Jet2_energy_gen,2)-pow(Jet1_px_gen+Jet2_px_gen,2)-pow(Jet1_py_gen+Jet2_py_gen,2)-pow(Jet1_pz_gen+Jet2_pz_gen,2));
      double x_minus_gen = ((Jet1_energy_gen-Jet1_pz_gen)+(Jet2_energy_gen-Jet2_pz_gen))/8000;
      double x_plus_gen = ((Jet1_energy_gen+Jet1_pz_gen)+(Jet2_energy_gen+Jet2_pz_gen))/8000;


      // Particle-flow
      double soma1 = 0;
      double soma2 = 0;
      double eta_max=-999.;
      double eta_min=999.;
      double cm = 8000;
      bool pf = false;
      bool pf_thresholds = false;

      for(vector<MyPFCand>::iterator it_pfcand = pFlow_coll->begin(); it_pfcand != pFlow_coll->end(); ++it_pfcand){
         int partType = it_pfcand->particleId;
         double eta = it_pfcand->Eta();
         double energy = it_pfcand->Energy();
         double pz = it_pfcand->Pz();

         // HF eta rings 29, 30, 40, 41
         //if( ( (fabs(eta) >= 2.866) && (fabs(eta) < 3.152) ) || (fabs(eta) >= 4.730) ) continue;

         // Apply thresholds
         if ( !pflowThreshold(*it_pfcand,thresholdsPFlow) ) continue;

         soma1 += (energy + pz);
         soma2 += (energy - pz);

         if (eta > eta_max) {eta_max = eta; PF_eta_max = true;}
         if (eta < eta_min) {eta_min = eta; PF_eta_min = true;}

       }

       double xi_plus_Reco = soma1/cm;
       double xi_minus_Reco = soma2/cm;
       double delta_eta_maxmin = eta_max - eta_min;


 
      //GenPart
      double genEPlusPz = 0;
      double genEMinusPz = 0;
     // double cm = 8000;
      Double_t proton_pi = 4000;
      Double_t proton_pz_plus=-999;
      Double_t proton_px_plus = -999.;
      Double_t proton_py_plus = -999.;
      Double_t proton_phi_plus = -999.;
      Double_t proton_energy_plus = 0.;
      Double_t proton_mass_minus = 0;
      Double_t proton_mass_plus = 0;
      Double_t proton_pz_minus=999;
      Double_t proton_px_minus = 999.;
      Double_t proton_py_minus = 999.;
      Double_t proton_pt_minus = 999.;
      Double_t proton_eta_minus = 999.;
      Double_t proton_phi_minus = 999.;
      Double_t proton_energy_minus = 0.;
      Double_t px_gen, pt_gen, mass_gen;
      Double_t py_gen;
      Double_t pz_gen;
      Double_t energy_gen;
      Double_t proton_pf;
      Double_t eta_gen;
      Double_t phi_gen;
      
      for(vector<MyGenPart>::iterator it_genpart = genPart->begin(); it_genpart != genPart->end(); ++it_genpart){
 
	 //double eta_gen = it_genpart->Eta();
         int status = it_genpart->status;
         int id = it_genpart->pdgId;
	 
	 if (status == 1) {
            energy_gen = it_genpart->Energy();
            px_gen = it_genpart->Px();
            py_gen = it_genpart->Py();
            pz_gen = it_genpart->Pz();
            pt_gen = it_genpart->Pt();
            eta_gen = it_genpart->Eta();
            phi_gen = it_genpart->Phi();
            mass_gen = it_genpart->M();
	    proton_pf = sqrt(px_gen*px_gen+py_gen*py_gen+pz_gen*pz_gen);  
	    if (id != 2212) {
   	       genEPlusPz += (energy_gen + pz_gen);
	       genEMinusPz += (energy_gen - pz_gen);
            }
	    if (id == 2212) {
             double pz_cut = 0.7*proton_pi;
             if (fabs(pz_gen) > pz_cut){

	        if (pz_gen > proton_pz_plus) {
                   //if (mc == "pythia8_diff" && !sd_plus_pythia) continue;
                    proton_pz_plus = pz_gen; 
                    //proton_pz_plus = sqrt(4000*4000-M_P*M_P); 
                    proton_px_plus = px_gen; proton_py_plus = py_gen; proton_phi_plus = phi_gen;     
                    proton_mass_plus = mass_gen; 
                    proton_energy_plus = sqrt(proton_px_plus*proton_px_plus + proton_py_plus*proton_py_plus + proton_pz_plus*proton_pz_plus + M_P*M_P);//energy_gen;
                    //proton_energy_plus = energy_gen;
                }
                if (pz_gen < proton_pz_minus) {
                   //if (mc == "pythia8_diff" && !sd_minus_pythia) continue;
                   proton_pz_minus = pz_gen; 
                   //proton_pz_minus = -sqrt(4000*4000-M_P*M_P); 
                   proton_px_minus = px_gen; proton_py_minus = py_gen;
                   proton_pt_minus = pt_gen; proton_mass_minus = mass_gen;
                   proton_eta_minus = eta_gen; proton_phi_minus = phi_gen; 
                   proton_energy_minus = sqrt(proton_px_minus*proton_px_minus + proton_py_minus*proton_py_minus + proton_pz_minus*proton_pz_minus + M_P*M_P);//energy_gen;
                   //proton_energy_minus = energy_gen;
                }
             }
            }
	 }
      }
      
      double xi_plus_gen = genEPlusPz/cm; //cout<<xi1_gen<<endl;
      double xi_minus_gen = genEMinusPz/cm;

      double xi_minus_proton_gen = -1.;
      double xi_minus_proton_smear_gen = -1.;
      double xi_minus_proton_smear_gen_gauss = -1.;
      double t_minus_proton_gen = 0.;
      double p_minus_proton_gen = 0.;
      double p_minus_proton_smear_gen = 0.;
      double t_minus_proton_smear_gen = 0.;
      double t_minus_proton_smear_gen_gauss = 0.;
      double thx_minus_proton = 0.;
      double thy_minus_proton = 0.;
      double thx_minus_proton_smear = 0.;
      double thy_minus_proton_smear = 0.;
      //double thx_proton_minus = 0.;
      //double thy_proton_minus = 0.;

      double xi_plus_proton_gen = -1.;
      double xi_plus_proton_smear_gen = -1.;
      double xi_plus_proton_smear_gen_gauss = -1.;
      double t_plus_proton_gen = 0.;
      double p_plus_proton_gen = 0.;
      double p_plus_proton_smear_gen = 0.;
      double t_plus_proton_smear_gen = 0.;
      double t_plus_proton_smear_gen_gauss = 0.;
      double thx_plus_proton = 0.;
      double thy_plus_proton = 0.;
      double thx_plus_proton_smear = 0.;
      double thy_plus_proton_smear = 0.;

      double proton_px_minus_smear = 0;
      double proton_py_minus_smear = 0;
      double proton_pz_minus_smear = 0;
      double proton_energy_minus_smear = 0;
      double proton_px_plus_smear = 0;
      double proton_py_plus_smear = 0;
      double proton_pz_plus_smear = 0;
      double proton_energy_plus_smear = 0;
      double v_x = 0;
      double v_y = 0;
      double v_z = 0;

      double t_plus_proton_rec = 0;
      double t_minus_proton_rec = 0;

     // generate vertex smearing
      vtx_smearing(v_x, v_y, v_z);

      //rp parametrization
      bool proton_minus_rp_accept_120 = false;
      bool proton_minus_rp_accept_121 = false;
      bool proton_minus_rp_accept_122 = false;
      bool proton_minus_rp_accept_123 = false;
      bool proton_minus_rp_accept_124 = false;
      bool proton_minus_rp_accept_125 = false;
      bool proton_minus_rp_accept_020 = false;

      bool proton_plus_rp_accept_020 = false;
      bool proton_plus_rp_accept_021 = false;
      bool proton_plus_rp_accept_022 = false;
      bool proton_plus_rp_accept_023 = false;
      bool proton_plus_rp_accept_024 = false;
      bool proton_plus_rp_accept_025 = false;
      bool proton_plus_rp_accept_120 = false;

      bool fiducial_cut_rp_024=false;
      bool fiducial_cut_rp_025=false;
      bool fiducial_cut_rp_124=false;
      bool fiducial_cut_rp_125=false;
      bool fiducial_cut_rp_124_smear=false;
      bool fiducial_cut_rp_125_smear=false;

      std::map<int,std::vector<double> > proton_plus_pars;
      std::map<int,std::vector<double> > proton_minus_pars;

      double xpos_24, ypos_24, xpos_25, ypos_25;
      if( proton_pz_plus > 0.){
         //beam smearing
         beam_smearing(proton_px_plus, proton_py_plus, proton_pz_plus, proton_energy_plus, proton_px_plus_smear, proton_py_plus_smear, proton_pz_plus_smear, proton_energy_plus_smear);
         TLorentzVector p_beam_plus;
         p_beam_plus.SetPxPyPzE (0, 0, sqrt(proton_pi*proton_pi - M_P*M_P), proton_pi);
         TLorentzVector p_beam_scatt_plus;
         p_beam_scatt_plus.SetPxPyPzE (proton_px_plus, proton_py_plus, proton_pz_plus, proton_energy_plus);
         TVector3 p_scatt_plus (proton_px_plus, proton_py_plus, proton_pz_plus);
         TLorentzVector t_vec_plus = (p_beam_plus - p_beam_scatt_plus);
         t_plus_proton_gen = t_vec_plus.Mag2();
         xi_plus_proton_gen =  ( 1 - (proton_pz_plus/proton_pi) );
         //thx_plus_proton = atan(-proton_px_plus/proton_pi);//p_beam_plus.Mag2());
         //thy_plus_proton = atan(proton_py_plus/proton_pi);//p_beam_plus.Mag2());
         double theta_plus = p_scatt_plus.Theta();//acos(proton_pz_plus/p_scatt_plus.Mag());
         double phi_plus = p_scatt_plus.Phi();//acos(proton_px_plus/sqrt(proton_px_plus*proton_px_plus+proton_py_plus*proton_py_plus));
         thx_plus_proton = theta_plus*cos(phi_plus);
         thy_plus_proton = theta_plus*sin(phi_plus);
         xi_plus_proton_smear_gen_gauss =  ( 1 - (proton_pz_plus_smear/proton_pi) );
         TLorentzVector p_beam_scatt_plus_smear (proton_px_plus_smear, proton_py_plus_smear, proton_pz_plus_smear, proton_energy_plus_smear);
         TLorentzVector t_vec_plus_smear = (p_beam_plus - p_beam_scatt_plus_smear);
         t_plus_proton_smear_gen_gauss = t_vec_plus_smear.Mag2();
         theta->Fill(theta_plus,1); 
         double delta_thx_plus = thx_plus_proton_smear-thx_plus_proton;

         xi_plus_proton_smear_gen = (400*delta_thx_plus) + xi_plus_proton_gen; 
         thx_plus_proton_smear = thx_plus_proton + gRandom->Gaus(0,25.10e-6);
         thy_plus_proton_smear = thy_plus_proton + gRandom->Gaus(0,2.42e-6);
         //t_plus_proton_smear_gen = -p_plus_proton_smear_gen*p_plus_proton_smear_gen*((thx_plus_proton_smear*thx_plus_proton_smear)+(thy_plus_proton_smear*thy_plus_proton_smear)); 
         double proton_px_plus_from_theta = -p_scatt_plus.Mag()*tan(thx_plus_proton_smear); 
         double proton_py_plus_from_theta = p_scatt_plus.Mag()*tan(thy_plus_proton_smear); 
         TLorentzVector p_beam_scatt_plus_smear_from_theta (proton_px_plus_from_theta, proton_py_plus_from_theta, proton_pz_plus_smear, proton_energy_plus_smear);
         TLorentzVector t_vec_plus_rec = (p_beam_plus - p_beam_scatt_plus_smear_from_theta);
         t_plus_proton_rec = t_vec_plus_rec.Mag2();
 
        //FIXME
         double out_x, out_thx, out_y, out_thy, out_xi;
         proton_plus_rp_accept_020 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 20, out_x, out_thx, out_y, out_thy, out_xi);
         proton_plus_pars[20] = std::vector<double>(5,0.);
         proton_plus_pars[20][0] = out_x; proton_plus_pars[20][1] = out_y;
         proton_plus_pars[20][2] = out_thx; proton_plus_pars[20][3] = out_thy;
         proton_plus_pars[20][4] = out_xi;

         proton_plus_rp_accept_024 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 24, out_x, out_thx, out_y, out_thy, out_xi);
         proton_plus_pars[24] = std::vector<double>(5,0.);
         proton_plus_pars[24][0] = out_x; proton_plus_pars[24][1] = out_y;
         proton_plus_pars[24][2] = out_thx; proton_plus_pars[24][3] = out_thy;
         proton_plus_pars[24][4] = out_xi;
         histosTH2F["pos_y_vs_x_proton_plus_024_025"]->Fill( proton_plus_pars[24][0], proton_plus_pars[24][1] , event_weight );
         fiducial_cut_rp_024 = proton_plus_pars[24][0]>0 && proton_plus_pars[24][0]<0.006 && proton_plus_pars[24][1]>0.0084 && proton_plus_pars[24][1]<0.029;
         //fiducial_cut_rp_024 = proton_plus_pars[24][0]>0 && proton_plus_pars[24][0]<0.006 && proton_plus_pars[24][1]>0.0082 && proton_plus_pars[24][1]<0.029;
      
         proton_plus_rp_accept_025 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 25, out_x, out_thx, out_y, out_thy, out_xi);
         proton_plus_pars[25] = std::vector<double>(5,0.);
         proton_plus_pars[25][0] = out_x; proton_plus_pars[25][1] = out_y;
         proton_plus_pars[25][2] = out_thx; proton_plus_pars[25][3] = out_thy;
         proton_plus_pars[25][4] = out_xi;
         histosTH2F["pos_y_vs_x_proton_plus_024_025"]->Fill( proton_plus_pars[25][0], proton_plus_pars[25][1] , event_weight );
         fiducial_cut_rp_025 = proton_plus_pars[25][0]>0 && proton_plus_pars[25][0]<0.006 && proton_plus_pars[25][1]<-0.0084 && proton_plus_pars[25][1]>-0.029;
         //fiducial_cut_rp_025 = proton_plus_pars[25][0]>0 && proton_plus_pars[25][0]<0.006 && proton_plus_pars[25][1]<-0.0082 && proton_plus_pars[25][1]>-0.029;

         proton_plus_rp_accept_021 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 21);
         proton_plus_rp_accept_022 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 22);
         proton_plus_rp_accept_023 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 23);
         proton_plus_rp_accept_024 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 24);
         proton_plus_rp_accept_025 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 25);
         proton_plus_rp_accept_020 = protonRPDetected(0., thx_plus_proton, 0., thy_plus_proton, -xi_plus_proton_gen, 20);

         if (fiducial_cut_rp_024 || fiducial_cut_rp_025){
             histosTH2F["pos_y_vs_x_proton_plus_024_025_accept"]->Fill( proton_plus_pars[24][0]*1000, proton_plus_pars[24][1]*1000 , event_weight );
             histosTH2F["pos_y_vs_x_proton_plus_024_025_accept"]->Fill( proton_plus_pars[25][0]*1000, proton_plus_pars[25][1]*1000 , event_weight );
         }
         xpos_24 = proton_plus_pars[24][0];
         ypos_24 = proton_plus_pars[24][1];
         xpos_25 = proton_plus_pars[25][0];
         ypos_25 = proton_plus_pars[25][1];
      }
 
      double t_minus_proton_gen_gauss, xpos_124, ypos_124, xpos_125, ypos_125;
      if( proton_pz_minus < 0.){
         //beam smearing
         beam_smearing(proton_px_minus, proton_py_minus, proton_pz_minus, proton_energy_minus, proton_px_minus_smear, proton_py_minus_smear, proton_pz_minus_smear, proton_energy_minus_smear);
     
         TLorentzVector p_beam_minus (0, 0, -sqrt(proton_pi*proton_pi - M_P*M_P), proton_pi);
         TLorentzVector p_beam_scatt_minus (proton_px_minus, proton_py_minus, proton_pz_minus, proton_energy_minus);
         TLorentzVector t_vec_minus = (p_beam_minus - p_beam_scatt_minus);
         t_minus_proton_gen = t_vec_minus.Mag2();
         xi_minus_proton_gen = (proton_pz_minus < 0.) ? ( 1 + (proton_pz_minus/proton_pi) ) : -1.;
         //thx_minus_proton = atan(-proton_px_minus/proton_pi);//p_beam_minus.Mag2());
         //thy_minus_proton = atan(proton_py_minus/proton_pi);//p_beam_minus.Mag2());
         TVector3 p_scatt_minus (proton_px_minus, proton_py_minus, -proton_pz_minus);
         double theta_minus = p_scatt_minus.Theta();
         double phi_minus = p_scatt_minus.Phi();
         thx_minus_proton = theta_minus*cos(phi_minus);
         thy_minus_proton = theta_minus*sin(phi_minus);
         xi_minus_proton_smear_gen_gauss = (proton_pz_minus_smear < 0.) ? ( 1 + (proton_pz_minus_smear/proton_pi) ) : -1.;
         TLorentzVector p_beam_scatt_minus_smear (proton_px_minus_smear, proton_py_minus_smear, proton_pz_minus_smear, proton_energy_minus_smear);
         TLorentzVector t_vec_minus_smear = (p_beam_minus - p_beam_scatt_minus_smear);
         t_minus_proton_smear_gen_gauss = t_vec_minus_smear.Mag2();
        //cout<<theta_minus<<" "<<phi_minus<<endl; 
         double delta_thx_minus = thx_minus_proton_smear-thx_minus_proton;
         xi_minus_proton_smear_gen = (400*delta_thx_minus) + xi_minus_proton_gen;
         thx_minus_proton_smear = thx_minus_proton + gRandom->Gaus(0,25.10e-6);
         thy_minus_proton_smear = thy_minus_proton + gRandom->Gaus(0,2.42e-6);
         //t_minus_proton_smear_gen = -p_minus_proton_smear_gen*p_minus_proton_smear_gen*((thx_minus_proton_smear*thx_minus_proton_smear)+(thy_minus_proton_smear*thy_minus_proton_smear)); 
         double proton_px_minus_from_theta = -p_scatt_minus.Mag()*tan(thx_minus_proton_smear);
         double proton_py_minus_from_theta = p_scatt_minus.Mag()*tan(thy_minus_proton_smear);
         TLorentzVector p_beam_scatt_minus_smear_from_theta (proton_px_minus_from_theta, proton_py_minus_from_theta, proton_pz_minus_smear, proton_energy_minus_smear);
         TLorentzVector t_vec_minus_rec = (p_beam_minus - p_beam_scatt_minus_smear_from_theta);
         t_minus_proton_rec = t_vec_minus_rec.Mag2();

         double out_x, out_thx, out_y, out_thy, out_xi;
         proton_minus_rp_accept_120 = protonRPDetected(v_x, thx_minus_proton, v_y, thy_minus_proton, -xi_minus_proton_gen, 120, out_x, out_thx, out_y, out_thy, out_xi);
         proton_minus_pars[120] = std::vector<double>(5,0.);
         proton_minus_pars[120][0] = out_x; proton_minus_pars[120][1] = out_y;
         proton_minus_pars[120][2] = out_thx; proton_minus_pars[120][3] = out_thy;
         proton_minus_pars[120][4] = out_xi;

         proton_minus_rp_accept_124 = protonRPDetected(v_x, thx_minus_proton, v_y, thy_minus_proton, -xi_minus_proton_gen, 124, out_x, out_thx, out_y, out_thy, out_xi);
         proton_minus_pars[124] = std::vector<double>(5,0.);
         proton_minus_pars[124][0] = out_x; proton_minus_pars[124][1] = out_y;
         proton_minus_pars[124][2] = out_thx; proton_minus_pars[124][3] = out_thy;
         proton_minus_pars[124][4] = out_xi;
         histosTH2F["pos_y_vs_x_proton_minus_124_125"]->Fill( proton_minus_pars[124][0], proton_minus_pars[124][1] , event_weight );
         fiducial_cut_rp_124 = proton_minus_pars[124][0]>0 && proton_minus_pars[124][0]<0.006 && proton_minus_pars[124][1]>0.0084 && proton_minus_pars[124][1]<0.027;
         fiducial_cut_rp_124_smear = proton_minus_pars[124][0]>0 && proton_minus_pars[124][0]<0.006 && proton_minus_pars[124][1]>0.0082 && proton_minus_pars[124][1]<0.027;//lower y bound by 200microm

         proton_minus_rp_accept_125 = protonRPDetected(v_x, thx_minus_proton, v_y, thy_minus_proton, -xi_minus_proton_gen, 125, out_x, out_thx, out_y, out_thy, out_xi);
         proton_minus_pars[125] = std::vector<double>(5,0.);
         proton_minus_pars[125][0] = out_x; proton_minus_pars[125][1] = out_y;
         proton_minus_pars[125][2] = out_thx; proton_minus_pars[125][3] = out_thy;
         proton_minus_pars[125][4] = out_xi;
         histosTH2F["pos_y_vs_x_proton_minus_124_125"]->Fill( proton_minus_pars[125][0], proton_minus_pars[125][1] , event_weight );
         fiducial_cut_rp_125 = proton_minus_pars[125][0]>0 && proton_minus_pars[125][0]<0.006 && proton_minus_pars[125][1]<-0.0084 && proton_minus_pars[125][1]>-0.027;
         fiducial_cut_rp_125_smear = proton_minus_pars[125][0]>0 && proton_minus_pars[125][0]<0.006 && proton_minus_pars[125][1]<-0.0082 && proton_minus_pars[125][1]>-0.027;

         if (fiducial_cut_rp_124 || fiducial_cut_rp_125){ 
             histosTH2F["pos_y_vs_x_proton_minus_124_125_accept"]->Fill( proton_minus_pars[124][0]*1000, proton_minus_pars[124][1]*1000 , event_weight );
             histosTH2F["pos_y_vs_x_proton_minus_124_125_accept"]->Fill( proton_minus_pars[125][0]*1000, proton_minus_pars[125][1]*1000 , event_weight );
         }

         proton_minus_rp_accept_121 = protonRPDetected(v_x, thx_minus_proton, v_y, thy_minus_proton, -xi_minus_proton_gen, 121);
         proton_minus_rp_accept_122 = protonRPDetected(v_x, thx_minus_proton, v_y, thy_minus_proton, -xi_minus_proton_gen, 122);
         proton_minus_rp_accept_123 = protonRPDetected(v_x, thx_minus_proton, v_y, thy_minus_proton, -xi_minus_proton_gen, 123);
         //proton_minus_rp_accept_124 = protonRPDetected(v_x, thx_proton_minus, v_y, thy_proton_minus, -xi_proton_minus_smear, 124);
         //proton_minus_rp_accept_125 = protonRPDetected(v_x, thx_proton_minus, v_y, thy_proton_minus, -xi_proton_minus_smear, 125);
         //proton_minus_rp_accept_120 = protonRPDetected(v_x, thx_proton_minus, v_y, thy_proton_minus, -xi_proton_minus_smear, 120);
         xpos_124 = proton_minus_pars[124][0];
         ypos_124 = proton_minus_pars[124][1];
         xpos_125 = proton_minus_pars[125][0];
         ypos_125 = proton_minus_pars[125][1];
      }

      //totem proton reconstructed
      double xi_proton_rec, xi_proton_smear_rec;
      double t_proton_rec, t_proton_smear_rec;
      double proton_beta_rec, proton_beta_gen;

      //gauss smearing
      float sigma_xi56=0.00720615 - 0.0418783*xi_minus_proton_smear_gen_gauss + 0.0999515*xi_minus_proton_smear_gen_gauss*xi_minus_proton_smear_gen_gauss; // sigma56 vs xi from Hubert
      xi_rec_proton_right_gauss = xi_minus_proton_smear_gen_gauss + gRandom->Gaus(0,sigma_xi56);
      double sigma_t56=0.233365*t_minus_proton_smear_gen_gauss - 0.0975751*t_minus_proton_smear_gen_gauss*t_minus_proton_smear_gen_gauss;  // sigma_t56 vs t from Hubert
      t_rec_proton_right_gauss = t_minus_proton_smear_gen_gauss + gRandom->Gaus(0,sigma_t56);
      float sigma_xi45 = 0.00714986 - 0.0408903*xi_plus_proton_smear_gen_gauss + 0.0965813*xi_plus_proton_smear_gen_gauss*xi_plus_proton_smear_gen_gauss;
      xi_rec_proton_left_gauss = xi_plus_proton_smear_gen_gauss + gRandom->Gaus(0,sigma_xi45);
      double sigma_t45 = 0.233365*t_plus_proton_smear_gen_gauss - 0.0975751*t_plus_proton_smear_gen_gauss*t_plus_proton_smear_gen_gauss;
      t_rec_proton_left_gauss = t_plus_proton_smear_gen_gauss + gRandom->Gaus(0,sigma_t45);
      

      //theta smearing
      double xi_minus_proton_smear_rec = xi_minus_proton_smear_gen;// + gRandom->Gaus(0,sigma_xi56);
      double proton_beta_minus_rec = x_minus_rec/xi_minus_proton_smear_rec;
      double proton_beta_minus_gen = x_minus_gen/xi_minus_proton_gen;
      double xi_plus_proton_smear_rec = xi_plus_proton_smear_gen;// + gRandom->Gaus(0,sigma_xi45_smear);
      double proton_beta_plus_rec = x_plus_rec/xi_plus_proton_smear_rec;
      double proton_beta_plus_gen = x_plus_gen/xi_plus_proton_gen;


      //rp_accept
      bool rp_minus_top = ( proton_minus_rp_accept_120 && proton_minus_rp_accept_124 && fiducial_cut_rp_124 );
      bool rp_minus_bottom = proton_minus_rp_accept_121 && proton_minus_rp_accept_125 && fiducial_cut_rp_125;
      bool proton_minus_rp_accept_mc = rp_minus_top || rp_minus_bottom; 
      bool rp_plus_top =  proton_plus_rp_accept_020 && proton_plus_rp_accept_024 && fiducial_cut_rp_024;
      bool rp_plus_bottom = proton_plus_rp_accept_021 && proton_plus_rp_accept_025 && fiducial_cut_rp_025 ;
      bool proton_plus_rp_accept_mc = rp_plus_top || rp_plus_bottom;
      bool proton_minus_rp_accept_mc_smear = ( proton_minus_rp_accept_120 && proton_minus_rp_accept_124 && fiducial_cut_rp_124_smear ) || ( proton_minus_rp_accept_121 && proton_minus_rp_accept_125 && fiducial_cut_rp_125_smear);// || ( proton_minus_rp_accept_122 && proton_minus_rp_accept_123 );

      bool rp_minus_sel =  !proton_plus_rp_accept_mc && proton_minus_rp_accept_mc;
      bool rp_plus_sel =  proton_plus_rp_accept_mc && !proton_minus_rp_accept_mc;

      rp_right = rp_minus_sel;
      rp_left = rp_plus_sel;
      rp_right_accep_top = proton_minus_rp_accept_120 && proton_minus_rp_accept_124; 
      rp_right_accep_bottom = proton_minus_rp_accept_121 && proton_minus_rp_accept_125;
      rp_left_accep_top = proton_plus_rp_accept_020 && proton_plus_rp_accept_024;
      rp_left_accep_bottom = proton_plus_rp_accept_021 && proton_plus_rp_accept_025;
      rp_xpos_24 = xpos_24;
      rp_ypos_24 = ypos_24;
      rp_xpos_25 = xpos_25;
      rp_ypos_25 = ypos_25;
      rp_xpos_124 = xpos_124;
      rp_ypos_124 = ypos_124;
      rp_xpos_125 = xpos_125;
      rp_ypos_125 = ypos_125;
      xi_rec_cms_right = xi_minus_Reco; 
      xi_gen_cms_right = xi_minus_gen; 
      xi_rec_cms_left = xi_plus_Reco; 
      xi_gen_cms_left = xi_plus_gen;
      xi_rec_proton_right = xi_minus_proton_smear_rec; 
      xi_rec_proton_left = xi_plus_proton_smear_rec; 
      xi_gen_proton_right = xi_minus_proton_gen; 
      xi_gen_proton_left = xi_plus_proton_gen; 
      t_rec_proton_right = t_minus_proton_rec; 
      t_rec_proton_left = t_plus_proton_rec; 
      t_gen_proton_right = t_minus_proton_gen; 
      t_gen_proton_left = t_plus_proton_gen; 
      x_rec_right = x_minus_rec; 
      x_rec_left = x_plus_rec; 
      x_gen_right = x_minus_gen; 
      x_gen_left = x_plus_gen;
      px_proton_right = proton_px_minus;
      px_proton_left = proton_px_plus;
      py_proton_right = proton_py_minus;
      py_proton_left = proton_py_plus;
      pz_proton_right = proton_pz_minus;
      pz_proton_left = proton_pz_plus;
     // px_proton_right_smear = proton_px_minus_smear;
     // px_proton_left_smear = proton_px_plus_smear;
     // py_proton_right_smear = proton_py_minus_smear;
     // py_proton_left_smear = proton_py_plus_smear;
      //pz_proton_right_smear = proton_pz_minus_smear;
      //pz_proton_left_smear = proton_pz_plus_smear;
      e_proton_right = proton_energy_minus; 
      e_proton_left = proton_energy_plus; 
     // e_proton_right_smear = proton_energy_minus_smear; 
     // e_proton_left_smear = proton_energy_plus_smear; 
      beta_rec_right = proton_beta_minus_rec; 
      beta_rec_left = proton_beta_plus_rec; 
      beta_gen_right = proton_beta_minus_gen; 
      beta_gen_left = proton_beta_plus_gen; 
      jet1_rec_pt = Jet1_pt_rec;      
      jet1_gen_pt = leadingJet_pt_gen;      
      jet1_rec_eta = Jet1_eta_rec;      
      jet1_gen_eta = Jet1_eta_gen;      
      jet1_rec_phi = Jet1_phi_rec;      
      jet1_gen_phi = Jet1_phi_gen;      
      jet2_rec_pt = Jet2_pt_rec;      
      jet2_gen_pt = secondJet_pt_gen;      
      jet2_rec_eta = Jet2_eta_rec;      
      jet2_gen_eta = Jet2_eta_gen;      
      jet2_rec_phi = Jet2_phi_rec;      
      jet2_gen_phi = Jet2_phi_gen;      
      weight_mc = event_weight;
      //theta_x_minus_smear =thx_minus_proton_smear; 
      //theta_y_minus_smear =thy_minus_proton_smear; 
      //theta_x_plus_smear =thx_plus_proton_smear; 
      //theta_y_plus_smear =thy_plus_proton_smear; 
      theta_x_minus =thx_minus_proton; 
      theta_y_minus =thy_minus_proton; 
      theta_x_plus =thx_plus_proton; 
      theta_y_plus =thy_plus_proton;
      mass_proton_left = proton_mass_plus; 
      mass_proton_right = proton_mass_minus; 
      small_tree->Fill();
     


    }//end loop for events
   file->Close();
 
  }//end of loop over files
//   cout <<"After selection " << nevents_accepted << endl;
  

   
  //output file
  TFile* output = new TFile(outputFileName,"RECREATE");
  output->cd();
  float cross_section;
  if (mc == "pomwig" && !reggeon && side_minus && side_plus) cross_section = 2*2.1189e7; //pb cross section for pomwig-pomeron
  if (mc == "pomwig" && !reggeon && ((side_minus && !side_plus)||(!side_minus && side_plus))) cross_section = 2.1189e7; //pb cross section for pomwig-pomeron
  if (mc == "pomwig" && reggeon && side_minus && side_plus) cross_section = 2*5.9695e6;//pb cross section for pomwig-reggeon
  if (mc == "pomwig" && reggeon && ((side_minus && !side_plus)||(!side_minus && side_plus))) cross_section = 5.9695e6;//pb cross section for pomwig-reggeon
  if (mc == "pythia8_diff") cross_section = 2.073e+10; //pb cross section pythia8

  float luminity_HLT_L1Jet1_198902 = 0.015879;//pb ---- luminity for LP_Jets1_Run2012C-PromptReco-v1-HighBetaJuly2012-Run198902
  float luminity_HLT_L1Jet2_198902 = 0.015879;//pb ---- luminity for LP_Jets2_Run2012C-PromptReco-v1-HighBetaJuly2012-Run198902
  float luminity_HLT_L1Jet1_198903 = 0.008698;//pb ---- luminity for LP_Jets2_Run2012C-PromptReco-v1-HighBetaJuly2012-Run198902
  float luminity_HLT_L1Jet2_198903 = 0.008698;//pb ---- luminity for LP_Jets2_Run2012C-PromptReco-v1-HighBetaJuly2012-Run198902
  float luminity = luminity_HLT_L1Jet1_198902 + luminity_HLT_L1Jet2_198902 + luminity_HLT_L1Jet1_198903 + luminity_HLT_L1Jet2_198903;
  
  float n_events = luminity*cross_section;
  float f1 = (float) nevents_total;
  // float f1 = (float) nweight_total;
  Double_t scale = n_events/f1;
  cout<<"eventos  "<<nevents_total<<"   pesos "<<nweight_total<<"   cross section: "<<cross_section<< "  scale "<< scale<< endl; 

  small_tree->Write();
  theta->Write();
  for(map<string,TH2F*>::iterator it_histo = histosTH2F.begin(); it_histo != histosTH2F.end(); ++it_histo)
     (*it_histo).second->Write();

  output->Close();
}
