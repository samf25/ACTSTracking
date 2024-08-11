#include "FilterTracksAlg.hxx"

// Gaudi
#include <k4Interface/IGeoSvc.h>

// DD4hep
#include <DD4hep/Detector.h>

// edm4hep
#include <edm4hep/Track.h>
#include <edm4hep/MutableTrack.h>

// ACTSTracking
#include "Helpers.hxx"

// Standard
#include <math.h>

DECLARE_COMPONENT(FilterTracksAlg)

// Implement constructor
FilterTracksAlg::FilterTracksAlg(const std::string& name, ISvcLocator* pSvcLocator) : Transformer(name, pSvcLocator,
		KeyValue("InputTrackCollectionName", "Tracks"),
		KeyValue("OutputTrackCollectionName", "FilteredTracks")) {}

StatusCode FilterTracksAlg::initialize() {
	// Set up magnetic field
	buildBfield();

	return StatusCode::SUCCESS;
}

// Build magnetic field
void FilterTracksAlg::buildBfield() {
	// Get GeoSvc
	ServiceHandle<IGeoSvc> geoSvc("GeoSvc", name());
	// Get magnetic field
	dd4hep::Detector* lcdd = geoSvc->getDetector();
	const double position[3] = {0, 0, 0};		// position to calculate magnetic field (here, the origin)
	double magneticFieldVector[3] = {0, 0, 0};	// initialise object to hold magnetic field
	lcdd->field().magneticField(
			position, magneticFieldVector);	// get the magnetic field vector from DD4hep
	m_Bz = magneticFieldVector[2] / dd4hep::tesla;
}

// Implement execute
edm4hep::TrackCollection FilterTracksAlg::operator()(const edm4hep::TrackCollection& tracks) const{
	// Create output collection
	edm4hep::TrackCollection outputTracks;

	// Filter tracks
	for (const auto& trk : tracks) {
		int nhittotal = trk.trackerHits_size();
		if (m_NHitsTotal > 0 && nhittotal <= m_NHitsTotal) continue; // Hit count check

		if (m_NHitsVertex > 0) {
			int nhitvertex = trk.getSubdetectorHitNumbers(1) + trk.getSubdetectorHitNumbers(2);	
			if (nhitvertex <= m_NHitsVertex) continue;
		}

		if (m_NHitsInner > 0) {
                        int nhitinner = trk.getSubdetectorHitNumbers(3) + trk.getSubdetectorHitNumbers(4);
                        if (nhitinner <= m_NHitsVertex) continue;
                }
		if (m_NHitsOuter > 0) {
                        int nhitouter = trk.getSubdetectorHitNumbers(5) + trk.getSubdetectorHitNumbers(6);
                        if (nhitouter <= m_NHitsVertex) continue;
                }

		float pt = fabs(0.3 * m_Bz / trk.getTrackStates(edm4hep::TrackState::AtIP).omega / 1000);
		if (m_MinPt > 0 && pt < m_MinPt) continue; // pT check

		// add tracks that pass all tests
		auto newTrk = outputTracks.create();
		ACTSTracking::makeMutableTrack(&trk, &newTrk);
	}

	return outputTracks;
}
