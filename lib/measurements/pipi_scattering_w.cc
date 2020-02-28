/*
 * pipi scattering
 * Authors:
 * Arjun Gambhir
 * Andre Walker-Loud
 * Jason Chang
 * David Brantley
 * Ben Horz
 * Haobo Yan
 * Do pipi scattering and write out the two-point correlator in hdf5
 */

// Deleted:
//if (paramtop.count("rotate_to_Dirac") != 0)
//else if (paramtop.count("mom_list") != 0)
//			
//			write(xml, "rotate_to_Dirac", par.rotate_to_Dirac);
//When write, 			if (par.is_mom_max == true)


#include "pipi_scattering_w.h"
#include "../contractions/pipi_scattering_func_w.h"
#include "meas/inline/abs_inline_measurement_factory.h"
//#include "../momentum/lalibe_sftmom.h"
#include "meas/inline/io/named_objmap.h"
#include "io/qprop_io.h"
#include "util/spin_basis.h"


namespace Chroma
{
	namespace LalibePipiScatteringEnv
	{
		namespace
		{
			AbsInlineMeasurement* createMeasurement(XMLReader& xml_in,
				const std::string& path)
			{
				return new InlineMeas(PipiParams(xml_in, path));
			}

			bool registered = false;
		}
		const std::string name = "PIPI_SCATTERING";

		bool registerAll()
		{
			bool success = true;
			if (!registered)
			{
				success &= TheInlineMeasurementFactory::Instance().registerObject(name, createMeasurement);
				registered = true;
			}
			return success;
		}

		void read(XMLReader& xml, const std::string& path, PipiParams::Param_t& par)
		{

			XMLReader paramtop(xml, path);

#ifdef BUILD_HDF5
			read(paramtop, "h5_file_name", par.file_name);
			read(paramtop, "obj_path", par.obj_path);
			QDPIO::cout << "HDF5 found, writing to" << par.file_name << " to path " << par.obj_path << std::endl;
#endif

			//We set output_full_correlator to true if no momentum is specified.
			read(paramtop, "p2_max", par.p2_max);
			par.output_full_correlator = false;
			QDPIO::cout << "Reading momenta centered around the origin with a max of " << par.p2_max << std::endl;
			read(paramtop, "particle_list", par.particle_list);
		}

		void write(XMLWriter& xml, const std::string& path, const PipiParams::Param_t& par){}

		void read(XMLReader& xml, const std::string& path, PipiParams::NamedObject_t& input)
		{

			XMLReader inputtop(xml, path);

			//read(inputtop, "gauge_id" , input.gauge_id);
			//Logic to read quark propagators (whichever ones are present.)

			if (inputtop.count("quark_1") != 0)
			{
				read(inputtop, "quark_1", input.quark_1);
				QDPIO::cout << "I found the first quark propagator, here is its id: " << input.quark_1 << std::endl;
				input.is_prop_1 = true;
			}
			else
			{
				QDPIO::cout << "I couldn't find the first quark, hope you don't need it for the inputted pipi scattering. " << std::endl;
				input.is_prop_1 = false;
			}
			if (inputtop.count("quark_2") != 0)
			{
				read(inputtop, "quark_2", input.quark_2);
				QDPIO::cout << "I found the second quark propagator, here is its id: " << input.quark_2 << std::endl;
				input.is_prop_2 = true;
			}
			else
			{
				QDPIO::cout << "I couldn't find the second quark, hope you don't need it for the inputted pipi scattering. " << std::endl;
				input.is_prop_2 = false;
			}
			if (inputtop.count("quark_3") != 0)
			{
				read(inputtop, "quark_3", input.quark_3);
				QDPIO::cout << "I found the third quark propagator, here is its id: " << input.quark_3 << std::endl;
				input.is_prop_3 = true;
			}
			else
			{
				QDPIO::cout << "I couldn't find the third quark, hope you don't need it for the inputted pipi scattering. " << std::endl;
				input.is_prop_3 = false;
			}
			if (inputtop.count("quark_4") != 0)
			{
				read(inputtop, "quark_4", input.quark_4);
				QDPIO::cout << "I found the forth quark propagator, here is its id: " << input.quark_4 << std::endl;
				input.is_prop_4 = true;
			}
			else
			{
				QDPIO::cout << "I couldn't find the forth quark, hope you don't need it for the inputted pipi scattering. " << std::endl;
				input.is_prop_4 = false;
			}

		}

		void write(XMLWriter& xml, const std::string& path, const PipiParams::NamedObject_t& input){}


		PipiParams::PipiParams()
		{
			frequency = 0;
		}

		PipiParams::PipiParams(XMLReader& xml_in, const std::string& path)
		{
			try
			{
				XMLReader paramtop(xml_in, path);

				if (paramtop.count("Frequency") == 1)
					read(paramtop, "Frequency", frequency);
				else
					frequency = 1;

				read(paramtop, "PipiParams", param);

				read(paramtop, "NamedObject", named_obj);

			}
			catch (const std::string& e)
			{
				QDPIO::cerr << __func__ << ": Caught Exception reading XML: " << e << std::endl;
				QDP_abort(1);
			}
		}

		void
			PipiParams::writeXML(XMLWriter& xml_out, const std::string& path){}


		void  InlineMeas::operator()(unsigned long update_no, XMLWriter& xml_out)
		{
			START_CODE();

			StopWatch snoop;
			snoop.reset();
			snoop.start();

			QDPIO::cout << "Pipi scattering starting..." << std::endl;

			//Grab all the propagators that are given.
			XMLReader prop_1_file_xml, prop_1_record_xml;
			LatticePropagator quark_propagator_1;
			XMLReader prop_2_file_xml, prop_2_record_xml;
			LatticePropagator quark_propagator_2;
			XMLReader prop_3_file_xml, prop_3_record_xml;
			LatticePropagator quark_propagator_3;
			XMLReader prop_4_file_xml, prop_4_record_xml;
			LatticePropagator quark_propagator_4;

			//Need origin, j_decay, and t0 for fourier transform!
			//Need j_decay of bc to know what comes with a minus sign.
			int j_decay;
			int t_0;
			multi1d<int> origin;

			if (params.named_obj.is_prop_1 == true)
			{
				QDPIO::cout << "Attempting to read the first propagator" << std::endl;
				try
				{
					quark_propagator_1 = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.quark_prop_1);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_1).getFileXML(prop_1_file_xml);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_1).getRecordXML(prop_1_record_file_xml);
					//Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
					MakeSourceProp_t  orig_header;
					if (prop_1_record_file_xml.count("/Propagator") != 0)
					{
						QDPIO::cout << "The first quark propagator is unsmeared, reading from Propagator tag..." << std::endl;
						read(prop_1_record_file_xml, "/Propagator", orig_header);
					}
					else if (prop_1_record_file_xml.count("/SinkSmear") != 0)
					{
						QDPIO::cout << "The first quark propagator is smeared, reading from SinkSmear tag..." << std::endl;
						read(prop_1_record_file_xml, "/SinkSmear", orig_header);
					}
					else
					{
						QDPIO::cout << "What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..." << std::endl;
					}

					j_decay = orig_header.source_header.j_decay;
					t_0 = orig_header.source_header.t_source;
					origin = orig_header.source_header.getTSrce();

				}
				catch (std::bad_cast)
				{
					QDPIO::cerr << name << ": caught dynamic cast error" << std::endl;
					QDP_abort(1);
				}
				catch (const std::string& e)
				{
					QDPIO::cerr << name << ": error reading src prop_header: "
						<< e << std::endl;
					QDP_abort(1);
				}
			}

			if (params.named_obj.is_prop_2 == true)
			{
				QDPIO::cout << "Attempting to read the first propagator" << std::endl;
				try
				{
					quark_propagator_2 = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.quark_prop_2);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_2).getFileXML(prop_2_file_xml);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_2).getRecordXML(prop_2_record_file_xml);
					//Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
					MakeSourceProp_t  orig_header;
					if (prop_2_record_file_xml.count("/Propagator") != 0)
					{
						QDPIO::cout << "The first quark propagator is unsmeared, reading from Propagator tag..." << std::endl;
						read(prop_2_record_file_xml, "/Propagator", orig_header);
					}
					else if (prop_2_record_file_xml.count("/SinkSmear") != 0)
					{
						QDPIO::cout << "The first quark propagator is smeared, reading from SinkSmear tag..." << std::endl;
						read(prop_2_record_file_xml, "/SinkSmear", orig_header);
					}
					else
					{
						QDPIO::cout << "What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..." << std::endl;
					}

					j_decay = orig_header.source_header.j_decay;
					t_0 = orig_header.source_header.t_source;
					origin = orig_header.source_header.getTSrce();

				}
				catch (std::bad_cast)
				{
					QDPIO::cerr << name << ": caught dynamic cast error" << std::endl;
					QDP_abort(1);
				}
				catch (const std::string& e)
				{
					QDPIO::cerr << name << ": error reading src prop_header: "
						<< e << std::endl;
					QDP_abort(1);
				}
			}

			if (params.named_obj.is_prop_3 == true)
			{
				QDPIO::cout << "Attempting to read the first propagator" << std::endl;
				try
				{
					quark_propagator_3 = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.quark_prop_3);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_3).getFileXML(prop_3_file_xml);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_3).getRecordXML(prop_3_record_file_xml);
					//Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
					MakeSourceProp_t  orig_header;
					if (prop_3_record_file_xml.count("/Propagator") != 0)
					{
						QDPIO::cout << "The first quark propagator is unsmeared, reading from Propagator tag..." << std::endl;
						read(prop_3_record_file_xml, "/Propagator", orig_header);
					}
					else if (prop_3_record_file_xml.count("/SinkSmear") != 0)
					{
						QDPIO::cout << "The first quark propagator is smeared, reading from SinkSmear tag..." << std::endl;
						read(prop_3_record_file_xml, "/SinkSmear", orig_header);
					}
					else
					{
						QDPIO::cout << "What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..." << std::endl;
					}

					j_decay = orig_header.source_header.j_decay;
					t_0 = orig_header.source_header.t_source;
					origin = orig_header.source_header.getTSrce();

				}
				catch (std::bad_cast)
				{
					QDPIO::cerr << name << ": caught dynamic cast error" << std::endl;
					QDP_abort(1);
				}
				catch (const std::string& e)
				{
					QDPIO::cerr << name << ": error reading src prop_header: "
						<< e << std::endl;
					QDP_abort(1);
				}
			}

			if (params.named_obj.is_prop_4 == true)
			{
				QDPIO::cout << "Attempting to read the first propagator" << std::endl;
				try
				{
					quark_propagator_4 = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.quark_prop_4);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_4).getFileXML(prop_4_file_xml);
					TheNamedObjMap::Instance().get(params.named_obj.quark_prop_4).getRecordXML(prop_4_record_file_xml);
					//Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
					MakeSourceProp_t  orig_header;
					if (prop_4_record_file_xml.count("/Propagator") != 0)
					{
						QDPIO::cout << "The first quark propagator is unsmeared, reading from Propagator tag..." << std::endl;
						read(prop_4_record_file_xml, "/Propagator", orig_header);
					}
					else if (prop_4_record_file_xml.count("/SinkSmear") != 0)
					{
						QDPIO::cout << "The first quark propagator is smeared, reading from SinkSmear tag..." << std::endl;
						read(prop_4_record_file_xml, "/SinkSmear", orig_header);
					}
					else
					{
						QDPIO::cout << "What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..." << std::endl;
					}

					j_decay = orig_header.source_header.j_decay;
					t_0 = orig_header.source_header.t_source;
					origin = orig_header.source_header.getTSrce();

				}
				catch (std::bad_cast)
				{
					QDPIO::cerr << name << ": caught dynamic cast error" << std::endl;
					QDP_abort(1);
				}
				catch (const std::string& e)
				{
					QDPIO::cerr << name << ": error reading src prop_header: "
						<< e << std::endl;
					QDP_abort(1);
				}
			}

			//Initialize FT stuff here, whether this is used or not below is another story...
			SftMom ft(params.param.p2_max, j_decay);

			//Here's Nt, we need this.
			int Nt = Layout::lattSize()[j_decay];

#ifdef BUILD_HDF5
			//If we are writing with hdf5, the start up is done here.
			HDF5Writer h5out(params.param.file_name);
			//h5out.push(params.param.obj_path);
			HDF5Base::writemode wmode;
			wmode = HDF5Base::ate;
#endif

			//Next we do the contractions for the specified particles.
			//This is going to be a horrible set of if statements for now, may change this later.
			//Loop over list of particles.

			for (int particle_index = 0; particle_index < params.param.particle_list.size(); particle_index++)
			{

				if (params.param.particle_list[particle_index] == "piplus")
				{
					QDPIO::cout << "Particle number " << (particle_index + 1) << " is the piplus." << std::endl;
					QDPIO::cout << "Checking to make sure we have the correct quark propagators to compute the piplus." << std::endl;
					if (params.named_obj.is_prop_1 == true && params.named_obj.is_prop_2 == true && params.named_obj.is_prop_3 == true && params.named_obj.is_prop_4 == true)
					{
						QDPIO::cout << "Found all four quarks for the two pions. Starting calculation..." << std::endl;


						multi3d<DComplex> correlator; // To be changed

						pipi_correlator(correlator, quark_propagator_1, quark_propagator_2, quark_propagator_3, quark_propagator_4, mom_list, origin_list, t0, j_decay);

						// Write out the correlator.
						write_correlator(params.param.output_full_correlator,
							"piplus",
#ifdef BUILD_HDF5
							params.param.obj_path, h5out, wmode,
#endif
							t_0, Nt, origin, ft, piplus);
					}
					else
						QDPIO::cout << "Sorry, I couldn't find all four quark. Skipping the pipi scattering..." << std::endl;
				}

			}

#ifdef BUILD_HDF5
			h5out.cd("/");
			h5out.close();
#endif

			//pop(xml_out);

			snoop.stop();
			QDPIO::cout << name << ": total time = "
				<< snoop.getTimeInSeconds()
				<< " secs" << std::endl;

			QDPIO::cout << name << ": ran successfully" << std::endl;

			END_CODE();
		}

	}

}
