#ifndef __pipi_scattering_h__
#define __pipi_scattering_h__

namespace Chroma
{

	namespace CorrelatorType
	{

		typedef std::tuple<int, int, int> momenta;
		typedef std::pair<momenta, momenta> momenta_pair;
		typedef std::map<const momenta_pair, multi1d<DComplex>> Correlator;

	} // namespace CorrelatorType

	void pipi_correlator(CorrelatorType::Correlator& correlator_out, const LatticePropagator& quark_prop_1, const LatticePropagator& quark_prop_2, const multi1d<int>& origin, const int p2max, const int ptot2max, const int t0, const int j_decay, const int diagram);
	void pik_correlator(CorrelatorType::Correlator& correlator_out, const LatticePropagator& quark_prop_1, const LatticePropagator& quark_prop_2, const multi1d<int>& origin, const int p2max, const int ptot2max, const int t0, const int j_decay, const int diagram);
	void pikQED_correlator(CorrelatorType::Correlator& correlator_out, const LatticePropagator& quark_prop_1, const LatticePropagator& quark_prop_2, const LatticePropagator& quark_prop_3, const multi1d<int>& origin, const int p2max, const int ptot2max, const int t0, const int j_decay, const int diagram);

} // End namespace Chroma

#endif
