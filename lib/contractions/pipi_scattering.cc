/*
ns that perform the contraction of quarks into the two-pion system.
 *  Based on qqbar_w.cc in Chroma
 *  Color is nested inside of spin.
 *  Authors:
 *  Haobo Yan
 *  Andre Walker-Loud
 *  Ben Horz
 */

#include "chromabase.h"
#include "pipi_scattering.h"
#include "../../../chroma/lib/meas/hadron/qqbar_w.cc"
#include <math.h>

namespace Chroma
{

	//void pion_correlator(multi1d<DComplex>& correlator, const LatticePropagator& quark_prop_1, const LatticePropagator& quark_prop_2, const multi2d<int>& mom_list, const multi1d<int>& origin_list, const int t0, const int j_decay)
	//{
	//	int G5 = Ns * Ns - 1;
	//	multi2d<DPropagator> single_pion; // function of (p, t)

	//	// Construct SftMom object using the argument 'mom_list' with the following constructor:
	//	// Constructor about origin, with a list of momenta 
	//	// SftMom(const multi2d<int> & moms, int j_decay = -1);
	//	SftMom phases(mom_list, j_decay);
	//	int Nt = phases.numSubsets(); // Length of lattice in j_decay direction by definition, but I understand it as the lattice number in time
	//	
	//	// Implement the momentum projection part using qqbar
	//	compute_qqbar(single_pion, quark_prop_1, quark_prop_2, phases, t0);

	//	// Loop over momenta
	//	for (int mom_num = 0; mom_num < phases.numMom(); ++mom_num)
	//		for (int t = 0; t < Nt; ++t)
	//		{
	//			// Contract colors and spins with gamma matrices
	//			correlator[mom_num] += trace(single_pion[mom_num][t] * Gamma(G5));
	//			// Fix the origin
	//			//correlator[mom_num] = correlator[mom_num] * exp();
	//		}
	//}


	void pipi_correlator(multi3d<DComplex>& correlator, const LatticePropagator& quark_prop_1, const LatticePropagator& quark_prop_2, const LatticePropagator& quark_prop_3, const LatticePropagator& quark_prop_4, const multi3d<int>& mom_list, const multi2d<int>& origin_list, const int t0, const int j_decay)
	{
		int G5 = Ns * Ns - 1;

		// Separate the mom_list
		multi2d<int> mom_list_1, mom_list_2;
		for (int p_num = 0; p_num < mom_list.size1(); ++p_num)
			for (int p_comp = 0; p_comp < mom_list.size3(); ++p_comp)
			{
				mom_list_1[p_num][p_comp] = mom_list[p_num][0][p_comp];
				mom_list_2[p_num][p_comp] = mom_list[p_num][1][p_comp];
			}

		bool same_origin = true;
		for (int o_comp = 0; o_comp < origin_list.size2(); ++o_comp)
			if (origin_list[0][o_comp] != origin_list[1][o_comp])
			{
				same_origin = false;
				break;
			}
		if (same_origin) // y=y'
		{
			multi2d<DPropagator> Q1, P1;

			SftMom phases1(mom_list_1, j_decay);
			SftMom phases2(mom_list_2, j_decay);
			int Nt = phases1.numSubsets();

			compute_qqbar(Q1, quark_prop_1, quark_prop_1, phases1, t0);
			
			DComplex trans_Q_to_P;
			//DComplex origin_fix;
			Double orgin_phases;
			P1 = Q1; // To be added, the relation is a ratio of two phase summation

			multi3d<DComplex> correlator1, correlator2, correlator3, correlator4;

			// Loop over all ps, p's and time
			for (int mom_num1 = 0; mom_num1 < phases1.numMom(); ++mom_num1)
				for (int mom_num2 = 0; mom_num2 < phases2.numMom(); ++mom_num2)
					for (int t = 0; t < Nt; ++t)
					{
						correlator1[mom_num1][mom_num2][t] += trace(Q1[mom_num1][t] * Gamma(G5)) * trace(P1[mom_num2][t] * Gamma(G5));

						correlator2[mom_num1][mom_num2][t] -= trace(Q1[mom_num1][t] * Gamma(G5) * P1[mom_num2][t] * Gamma(G5)); // I use - because of the minus sign picked up by Grossman numbers

						correlator3[mom_num1][mom_num2][t] -= trace(Q1[mom_num1][t] * Gamma(G5) * P1[mom_num2][t] * Gamma(G5));

						correlator4[mom_num1][mom_num2][t] += trace(Q1[mom_num1][t] * Gamma(G5)) * trace(P1[mom_num2][t] * Gamma(G5));

						correlator[mom_num1][mom_num2][t] += correlator1[mom_num1][mom_num2][t] + correlator2[mom_num1][mom_num2][t] + correlator3[mom_num1][mom_num2][t] + correlator4[mom_num1][mom_num2][t];
						// Fix the origin
						orgin_phases = 0;
						multi1d<int> fix_mom1, fix_mom2;
						fix_mom1 = phases1.numToMom(mom_num1);
						fix_mom2 = phases2.numToMom(mom_num2);
						for (int p_comp = 0; p_comp < mom_list.size3(); ++p_comp)
							orgin_phases += fix_mom1[p_comp] * origin_list[0][p_comp] + fix_mom2[p_comp] * origin_list[1][p_comp];
						DComplex origin_fix = cmplx(cos(orgin_phases), sin(orgin_phases));
						correlator[mom_num1][mom_num2][t] *= origin_fix;
					}
		}
		else // The general case
		{
			multi2d<DPropagator> Q1, P1, Q2, P2, Q3, P3, Q4, P4;

			SftMom phases1(mom_list_1, j_decay);
			SftMom phases2(mom_list_2, j_decay);
			int Nt = phases1.numSubsets();

			compute_qqbar(Q1, quark_prop_1, quark_prop_1, phases1, t0);
			compute_qqbar(P1, quark_prop_2, quark_prop_2, phases2, t0);
			compute_qqbar(Q2, quark_prop_1, quark_prop_3, phases1, t0);
			compute_qqbar(P2, quark_prop_2, quark_prop_4, phases2, t0);
			compute_qqbar(Q3, quark_prop_3, quark_prop_1, phases1, t0);
			compute_qqbar(P3, quark_prop_4, quark_prop_2, phases2, t0);
			compute_qqbar(Q4, quark_prop_3, quark_prop_3, phases1, t0);
			compute_qqbar(P4, quark_prop_4, quark_prop_4, phases2, t0);

			//DComplex origin_fix;
			Double orgin_phases;

			multi3d<DComplex> correlator1, correlator2, correlator3, correlator4;

			for (int mom_num1 = 0; mom_num1 < phases1.numMom(); ++mom_num1)
				for (int mom_num2 = 0; mom_num2 < phases2.numMom(); ++mom_num2)
					for (int t = 0; t < Nt; ++t)
					{
						correlator1[mom_num1][mom_num2][t] += trace(Q1[mom_num1][t] * Gamma(G5)) * trace(P1[mom_num2][t] * Gamma(G5));

						correlator2[mom_num1][mom_num2][t] -= trace(Q2[mom_num1][t] * Gamma(G5) * P2[mom_num2][t] * Gamma(G5)); // I use - because of the minus sign picked up by Grossman numbers

						correlator3[mom_num1][mom_num2][t] -= trace(Q3[mom_num1][t] * Gamma(G5) * P3[mom_num2][t] * Gamma(G5));

						correlator4[mom_num1][mom_num2][t] += trace(Q4[mom_num1][t] * Gamma(G5)) * trace(P4[mom_num2][t] * Gamma(G5));

						correlator[mom_num1][mom_num2][t] += correlator1[mom_num1][mom_num2][t] + correlator2[mom_num1][mom_num2][t] + correlator3[mom_num1][mom_num2][t] + correlator4[mom_num1][mom_num2][t];
						// Fix the origin
						orgin_phases = 0;
						multi1d<int> fix_mom1, fix_mom2;
						fix_mom1 = phases1.numToMom(mom_num1);
						fix_mom2 = phases2.numToMom(mom_num2);
						for (int p_comp = 0; p_comp < mom_list.size3(); ++p_comp)
							orgin_phases += fix_mom1[p_comp] * origin_list[0][p_comp] + fix_mom2[p_comp] * origin_list[1][p_comp];
						DComplex origin_fix = cmplx(cos(orgin_phases), sin(orgin_phases));
						correlator[mom_num1][mom_num2][t] *= origin_fix;
					}

		}

	}


} // End namespace Chroma

