// -------------------------------------------------
// shower analysis module
//
// Author: Rory Fitzpatrick (roryfitz@umich.edu)
// Created: 7/16/18
// -------------------------------------------------

// Framework includes
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Common/FindManyP.h"

#include "larcore/Geometry/Geometry.h"
#include "lardata/Utilities/AssociationUtil.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCFlux.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "lardataobj/AnalysisBase/ParticleID.h"
#include "lardataobj/MCBase/MCDataHolder.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/Simulation/SimChannel.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"

#include "TTree.h"
#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include <TROOT.h>
#include <TStyle.h>

namespace shower {

  class TCShowerAnalysis : public art::EDAnalyzer {

  public:

    explicit TCShowerAnalysis(fhicl::ParameterSet const& pset);
    virtual ~TCShowerAnalysis();

    void reconfigure(fhicl::ParameterSet const& pset);
    void beginJob();
    void analyze(const art::Event& evt);
   
    void showerProfile(std::vector< art::Ptr<recob::Hit> > showerhits, TVector3 shwvtx, TVector3 shwdir, double elep);
    void showerProfileTrue(std::vector< art::Ptr<recob::Hit> > allhits, double elep);
    void showerProfileTrue(std::vector< art::Ptr<sim::SimChannel> > allchan, simb::MCParticle electron);
 
  protected:

  private: 

    void resetVars();

    TTree* fTree;

    TProfile* fShowerProfileSim;
    TProfile* fShowerProfileHit;
    TProfile* fShowerProfileReco;

    TProfile2D* fShowerProfileSim2D;
    TProfile2D* fShowerProfileHit2D;
    TProfile2D* fShowerProfileReco2D;

    TProfile* fShowerProfileSimTrans;
    TProfile* fShowerProfileHitTrans;
    TProfile* fShowerProfileRecoTrans;

    TProfile2D* fShowerProfileSimTrans2D;
    TProfile2D* fShowerProfileHitTrans2D;
    TProfile2D* fShowerProfileRecoTrans2D;

    const int NBINS = 20;
    const int TMAX = 5;

    std::string fHitModuleLabel;
    std::string fShowerModuleLabel;
    std::string fCalorimetryModuleLabel;
    std::string fGenieGenModuleLabel;
    std::string fDigitModuleLabel;

    calo::CalorimetryAlg fCalorimetryAlg;

  }; // class TCShowerAnalysis

} // shower

// -------------------------------------------------

shower::TCShowerAnalysis::TCShowerAnalysis(fhicl::ParameterSet const& pset) :
  EDAnalyzer(pset),
  fHitModuleLabel           (pset.get< std::string >("HitModuleLabel", "trajcluster" ) ),
  fShowerModuleLabel        (pset.get< std::string >("ShowerModuleLabel", "tcshower" ) ),
  fCalorimetryModuleLabel   (pset.get< std::string >("CalorimetryModuleLabel", "calo")  ), 
  fGenieGenModuleLabel      (pset.get< std::string >("GenieGenModuleLabel", "generator")  ),
  fDigitModuleLabel         (pset.get< std::string >("DigitModuleLabel", "largeant")  ),
  fCalorimetryAlg           (pset.get< fhicl::ParameterSet >("CalorimetryAlg") ) {
  this->reconfigure(pset);
} // TCShowerAnalysis

// -------------------------------------------------

shower::TCShowerAnalysis::~TCShowerAnalysis() {
} // ~TCShowerAnalysis

// -------------------------------------------------

void shower::TCShowerAnalysis::reconfigure(fhicl::ParameterSet const& pset) {
} // reconfigure

// -------------------------------------------------

void shower::TCShowerAnalysis::beginJob() {
  art::ServiceHandle<art::TFileService> tfs;
  fTree = tfs->make<TTree>("tcshowerana", "tcshowerana");
  fShowerProfileSim = tfs->make<TProfile>("fShowerProfileSim", "longitudinal e- profile (true, simchannel);t;E (MeV)", NBINS, 0, TMAX);
  fShowerProfileHit = tfs->make<TProfile>("fShowerProfileHit", "longitudinal e- profile (true, hit);t;E (MeV)", NBINS, 0, TMAX);
  fShowerProfileReco = tfs->make<TProfile>("fShowerProfileReco", "longitudinal e- profile (reco);t;Q", NBINS, 0, TMAX);

  fShowerProfileSim2D = tfs->make<TProfile2D>("fShowerProfileSim2D", "longitudinal e- profile (true, simchannel);t;electron energy (MeV);E (MeV)", NBINS, 0, TMAX, 10, 0.5, 10.5);
  fShowerProfileHit2D = tfs->make<TProfile2D>("fShowerProfileHit2D", "longitudinal e- profile (true, hit);t;electron energy (MeV);E (MeV)", NBINS, 0, TMAX, 10, 0.5, 10.5);
  fShowerProfileReco2D = tfs->make<TProfile2D>("fShowerProfileReco2D", "longitudinal e- profile (reco);t;electron energy (MeV);Q", NBINS, 0, TMAX, 10, 0.5, 10.5);

  fShowerProfileSimTrans = tfs->make<TProfile>("fShowerProfileSimTrans", "transverse e- profile (true, simchannel);dist (cm);E (MeV)", NBINS, -5, 5);
  fShowerProfileHitTrans = tfs->make<TProfile>("fShowerProfileHitTrans", "transverse e- profile (true, hit);dist (cm);E (MeV)", NBINS, -5, 5);
  fShowerProfileRecoTrans = tfs->make<TProfile>("fShowerProfileRecoTrans", "transverse e- profile (reco);dist (cm);Q", NBINS, -5, 5);
  
  fShowerProfileSimTrans2D = tfs->make<TProfile2D>("fShowerProfileSimTrans2D", "longitudinal e- profile (true, simchannel);t;electron energy (MeV);E (MeV)", NBINS, -5, 5, 10, 0.5, 10.5);
  fShowerProfileHitTrans2D = tfs->make<TProfile2D>("fShowerProfileHitTrans2D", "longitudinal e- profile (true, hit);t;electron energy (MeV);E (MeV)", NBINS, -5, 5, 10, 0.5, 10.5);
  fShowerProfileRecoTrans2D = tfs->make<TProfile2D>("fShowerProfileRecoTrans2D", "longitudinal e- profile (reco);t;electron energy (MeV);Q", NBINS, -5, 5, 10, 0.5, 10.5);

} // beginJob

// -------------------------------------------------

void shower::TCShowerAnalysis::analyze(const art::Event& evt) {

  resetVars();

  art::Handle< std::vector<recob::Hit> > hitListHandle;
  std::vector<art::Ptr<recob::Hit> > hitlist;
  if (evt.getByLabel(fHitModuleLabel,hitListHandle))
    art::fill_ptr_vector(hitlist, hitListHandle);

  art::Handle< std::vector<sim::SimChannel> > scListHandle;
  std::vector<art::Ptr<sim::SimChannel> > simchanlist;
  if (evt.getByLabel(fDigitModuleLabel,scListHandle))
    art::fill_ptr_vector(simchanlist, scListHandle);

  art::Handle< std::vector<recob::Shower> > showerListHandle;
  std::vector<art::Ptr<recob::Shower> > showerlist;
  if (evt.getByLabel(fShowerModuleLabel,showerListHandle))
    art::fill_ptr_vector(showerlist, showerListHandle);
  
  // mc info
  art::Handle< std::vector<simb::MCTruth> > mctruthListHandle;
  std::vector<art::Ptr<simb::MCTruth> > mclist;
  if (evt.getByLabel(fGenieGenModuleLabel,mctruthListHandle))
    art::fill_ptr_vector(mclist, mctruthListHandle);

  art::FindManyP<recob::Hit> shwfm(showerListHandle, evt, fShowerModuleLabel);

  if (mclist.size()) {
    art::Ptr<simb::MCTruth> mctruth = mclist[0];
    if (mctruth->NeutrinoSet()) {
      if (std::abs(mctruth->GetNeutrino().Nu().PdgCode()) == 12 && mctruth->GetNeutrino().CCNC() == 0) {
	double elep =  mctruth->GetNeutrino().Lepton().E();
	std::cout << "ELECTRON ENERGY: " << elep << std::endl;
	if (showerlist.size()) {
	  std::vector< art::Ptr<recob::Hit> > showerhits = shwfm.at(0);
	  showerProfile(showerhits, showerlist[0]->ShowerStart(), showerlist[0]->Direction(), elep);
	}
	showerProfileTrue(hitlist, elep);
	showerProfileTrue(simchanlist, mctruth->GetNeutrino().Lepton());
      }
    }
  }

  fTree->Fill();

} // analyze

// -------------------------------------------------

void shower::TCShowerAnalysis::resetVars() {

} // resetVars

// -------------------------------------------------

void shower::TCShowerAnalysis::showerProfile(std::vector< art::Ptr<recob::Hit> > showerhits, TVector3 shwvtx, TVector3 shwdir, double elep) {

  auto const* detprop = lar::providerFrom<detinfo::DetectorPropertiesService>();
  art::ServiceHandle<geo::Geometry> geom;

  auto collectionPlane = geo::PlaneID(0, 0, 1);

  double shwVtxTime = detprop->ConvertXToTicks(shwvtx[0], collectionPlane);
  double shwVtxWire = geom->WireCoordinate(shwvtx[1], shwvtx[2], collectionPlane);

  double shwTwoTime = detprop->ConvertXToTicks(shwvtx[0]+shwdir[0], collectionPlane);
  double shwTwoWire = geom->WireCoordinate(shwvtx[1]+shwdir[1], shwvtx[2]+shwdir[2], collectionPlane);

  TH1F* temp = new TH1F("temp", "temp", NBINS, 0, TMAX);
  TH1D* temp2 = new TH1D("temp2", "temp2", NBINS, -5, 5);

  for (size_t i = 0; i < showerhits.size(); ++i) {
    if (showerhits[i]->WireID().Plane != collectionPlane.Plane) continue;

    double wirePitch = geom->WirePitch(showerhits[i]->WireID());
    double tickToDist = detprop->DriftVelocity(detprop->Efield(),detprop->Temperature());
    tickToDist *= 1.e-3 * detprop->SamplingRate(); // 1e-3 is conversion of 1/us to 1/ns

    double xvtx = shwVtxTime * tickToDist;
    double yvtx = shwVtxWire * wirePitch;

    double xtwo = shwTwoTime * tickToDist;
    double ytwo = shwTwoWire * wirePitch;

    double xtwoorth = (ytwo - yvtx) + xvtx;
    double ytwoorth = -(xtwo - xvtx) + yvtx;

    double xhit = showerhits[i]->PeakTime() * tickToDist;
    double yhit = showerhits[i]->WireID().Wire * wirePitch;

    double dist = std::abs((ytwoorth-yvtx)*xhit - (xtwoorth-xvtx)*yhit + xtwoorth*yvtx - ytwoorth*xvtx)/std::sqrt( pow((ytwoorth-yvtx), 2) + pow((xtwoorth-xvtx), 2) );

    double tdist = ((ytwo-yvtx)*xhit - (xtwo-xvtx)*yhit + xtwo*yvtx - ytwo*xvtx)/std::sqrt( pow((ytwo-yvtx), 2) + pow((xtwo-xvtx), 2) );

    //    double dist = sqrt(pow(xhit-xvtx, 2) + pow(yhit-yvtx, 2));
    
    double to3D = 1. / sqrt( pow(xvtx-xtwo,2) + pow(yvtx-ytwo,2) ) ; // distance between two points in 3D space is one 
    dist *= to3D;
    tdist *= to3D;
    double Q = showerhits[i]->Integral() * fCalorimetryAlg.LifetimeCorrection(showerhits[i]->PeakTime());
    double t = dist / 14; // convert to radiation lengths    

    temp->Fill(t, Q);
    temp2->Fill(tdist, Q);

  } // loop through showerhits

  for (int i = 0; i < NBINS; ++i) {
    if (temp->GetBinContent(i+1) == 0) continue;
    //    if (temp->GetBinContent(i+2) == 0) continue;
    fShowerProfileReco->Fill(temp->GetBinCenter(i+1), temp->GetBinContent(i+1));
    fShowerProfileReco2D->Fill(temp->GetBinCenter(i+1), elep, temp->GetBinContent(i+1));
  }

  for (int i = 0; i < NBINS; ++i) {
    if (temp2->GetBinContent(i+1) == 0) continue;
    fShowerProfileRecoTrans->Fill(temp2->GetBinCenter(i+1), temp2->GetBinContent(i+1));
    fShowerProfileRecoTrans2D->Fill(temp2->GetBinCenter(i+1), elep, temp2->GetBinContent(i+1));
  }

} // showerProfile

// -------------------------------------------------

void shower::TCShowerAnalysis::showerProfileTrue(std::vector< art::Ptr<recob::Hit> > allhits, double elep) {

  auto const* detprop = lar::providerFrom<detinfo::DetectorPropertiesService>();
  art::ServiceHandle<geo::Geometry> geom;
  auto collectionPlane = geo::PlaneID(0, 0, 1);
  art::ServiceHandle<cheat::BackTrackerService> btserv;
  art::ServiceHandle<cheat::ParticleInventoryService> piserv;
  std::map<int,double> trkID_E;  

  TH1F* temp = new TH1F("temp", "temp", NBINS, 0, TMAX);
  TH1D* temp2 = new TH1D("temp2", "temp2", NBINS, -5, 5);

  double xvtx = -999;
  double yvtx = -999;
  double zvtx = -999;
  double xtwo = -999;
  double ytwo = -999;
  double ztwo = -999;
  double shwvtxT = -999;
  double shwvtxW = -999;
  double shwtwoT = -999;
  double shwtwoW = -999;

  double shwvtxx = -999;
  double shwvtxy = -999;
  double shwtwox = -999;
  double shwtwoy = -999;
  double xtwoorth = -999;
  double ytwoorth = -999;

  double wirePitch = -999;
  double tickToDist = -999;

  bool foundParent = false;

  for (size_t i = 0; i < allhits.size(); ++i) {
    if (allhits[i]->WireID().Plane != collectionPlane.Plane) continue;

    art::Ptr<recob::Hit> hit = allhits[i];
    std::vector<sim::TrackIDE> trackIDs = btserv->HitToEveTrackIDEs(hit);

    for (size_t j = 0; j < trackIDs.size(); ++j) {
      // only want energy associated with the electron and electron must have neutrino mother
      if ( std::abs((piserv->TrackIdToParticle_P(trackIDs[j].trackID))->PdgCode()) != 11) continue;
      if ( std::abs((piserv->TrackIdToParticle_P(trackIDs[j].trackID))->Mother()) != 0) continue;

      if (!foundParent) {
	xvtx = (piserv->TrackIdToParticle_P(trackIDs[j].trackID))->Vx();
	yvtx = (piserv->TrackIdToParticle_P(trackIDs[j].trackID))->Vy();
	zvtx = (piserv->TrackIdToParticle_P(trackIDs[j].trackID))->Vz();

	xtwo = xvtx + (piserv->TrackIdToParticle_P(trackIDs[j].trackID))->Px();
	ytwo = yvtx + (piserv->TrackIdToParticle_P(trackIDs[j].trackID))->Py();
	ztwo = zvtx + (piserv->TrackIdToParticle_P(trackIDs[j].trackID))->Pz();

	shwvtxT = detprop->ConvertXToTicks(xvtx, collectionPlane);
	shwvtxW = geom->WireCoordinate(yvtx, zvtx, collectionPlane);

	shwtwoT = detprop->ConvertXToTicks(xtwo, collectionPlane);
	shwtwoW = geom->WireCoordinate(ytwo, ztwo, collectionPlane);

	wirePitch = geom->WirePitch(hit->WireID());
	tickToDist = detprop->DriftVelocity(detprop->Efield(),detprop->Temperature());
	tickToDist *= 1.e-3 * detprop->SamplingRate(); // 1e-3 is conversion of 1/us to 1/ns

	shwvtxx = shwvtxT * tickToDist;
	shwvtxy = shwvtxW * wirePitch;

	shwtwox = shwtwoT * tickToDist;
	shwtwoy = shwtwoW * wirePitch;

	xtwoorth = (shwtwoy - shwvtxy) + shwvtxx;
	ytwoorth = -(shwtwox - shwvtxx) + shwvtxy;

	foundParent = true;
      }
      double xhit = hit->PeakTime() * tickToDist;
      double yhit = hit->WireID().Wire * wirePitch;

      double dist = abs((ytwoorth-shwvtxy)*xhit - (xtwoorth-shwvtxx)*yhit + xtwoorth*shwvtxy - ytwoorth*shwvtxx)/sqrt( pow((ytwoorth-shwvtxy), 2) + pow((xtwoorth-shwvtxx), 2) );

      double tdist = ((shwtwoy-shwvtxy)*xhit - (shwtwox-shwvtxx)*yhit + shwtwox*shwvtxy - shwtwoy*shwvtxx)/sqrt( pow((shwtwoy-shwvtxy), 2) + pow((shwtwox-shwvtxx), 2) );

      double to3D = sqrt( pow(xvtx-xtwo , 2) + pow(yvtx-ytwo , 2) + pow(zvtx-ztwo , 2) ) / sqrt( pow(shwvtxx-shwtwox,2) + pow(shwvtxy-shwtwoy,2) ) ; // distance between two points in 3D space is one 
      dist *= to3D;
      tdist *= to3D;
      double E = trackIDs[j].energy;
      double t = dist / 14; // convert to radiation lengths

      temp->Fill(t, E);
      temp2->Fill(tdist, E);

      break;
    } // loop through track IDE

  } // loop through all hits

  for (int i = 0; i < NBINS; ++i) {
    if (temp->GetBinContent(i+1) == 0) continue;
    fShowerProfileHit->Fill(temp->GetBinCenter(i+1), temp->GetBinContent(i+1));
    fShowerProfileHit2D->Fill(temp->GetBinCenter(i+1), elep, temp->GetBinContent(i+1));
  }

  for (int i = 0; i < NBINS; ++i) {
    if (temp2->GetBinContent(i+1) == 0) continue;
    fShowerProfileHitTrans->Fill(temp2->GetBinCenter(i+1), temp2->GetBinContent(i+1));
    fShowerProfileHitTrans2D->Fill(temp2->GetBinCenter(i+1), elep, temp2->GetBinContent(i+1));
  }

  return;
} // showerProfileTrue

// -------------------------------------------------

void shower::TCShowerAnalysis::showerProfileTrue(std::vector< art::Ptr<sim::SimChannel> > allchan, simb::MCParticle electron) {
  art::ServiceHandle<cheat::ParticleInventoryService> piserv;
  art::ServiceHandle<geo::Geometry> geom;

  std::vector<sim::MCEnDep> alledep;

  TH1D* temp = new TH1D("temp", "temp", NBINS, 0, TMAX);
  TH1D* temp2 = new TH1D("temp2", "temp2", NBINS, -5, 5);

  for (size_t i = 0; i < allchan.size(); ++i) {
    art::Ptr<sim::SimChannel> simchan = allchan[i];
    if ( geom->View(simchan->Channel()) != geo::kV ) continue;
    auto tdc_ide_map = simchan->TDCIDEMap();

    for(auto const& tdc_ide_pair : tdc_ide_map) {
      for (auto const& ide : tdc_ide_pair.second) {
	if (piserv->TrackIdToMotherParticle_P(ide.trackID) == NULL) continue;
	if ( std::abs(piserv->TrackIdToMotherParticle_P(ide.trackID)->PdgCode()) != 11 ) continue;

	sim::MCEnDep edep;
	edep.SetVertex(ide.x,ide.y,ide.z);
	edep.SetEnergy(ide.energy);
	edep.SetTrackId(ide.trackID);
	
	alledep.push_back(edep);
	
      } // loop through ide
    } // loop through tdc_ide

  } // loop through channels

  double x0 = electron.Vx();
  double y0 = electron.Vy();
  double z0 = electron.Vz();

  double x2 = electron.Px();
  double y2 = electron.Py();
  double z2 = electron.Pz();

  TVector3 v0(x2, y2, z2);

  v0 = v0.Unit();
  TVector3 v0orth = v0.Orthogonal();

  for (size_t i = 0; i < alledep.size(); ++i) {
    double x = (double)alledep[i].Vertex()[0];
    double y = (double)alledep[i].Vertex()[1];
    double z = (double)alledep[i].Vertex()[2];

    TVector3 v1(x-x0, y-y0, z-z0);

    double dist = v0.Dot(v1);

    double E = alledep[i].Energy();
    double t = dist / 14; // convert to radiation lengths                                                               

    temp->Fill(t, E);

    temp2->Fill(v0orth.Dot(v1), E);

  }
  
  for (int i = 0; i < NBINS; ++i) {
    if (temp->GetBinContent(i+1) == 0) continue;
    fShowerProfileSim->Fill(temp->GetBinCenter(i+1), temp->GetBinContent(i+1));
    fShowerProfileSim2D->Fill(temp->GetBinCenter(i+1), electron.E(), temp->GetBinContent(i+1));
  }

  for (int i = 0; i < NBINS; ++i) {
    if (temp2->GetBinContent(i+1) == 0) continue;
    fShowerProfileSimTrans->Fill(temp2->GetBinCenter(i+1), temp2->GetBinContent(i+1));
    fShowerProfileSimTrans2D->Fill(temp2->GetBinCenter(i+1), electron.E(), temp2->GetBinContent(i+1));
  }

  return;

} // showerProfileTrue 

// -------------------------------------------------

DEFINE_ART_MODULE(shower::TCShowerAnalysis)
