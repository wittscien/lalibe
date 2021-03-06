/*
 * Baryon contractions that intercept normal quark lines with an FH propagator.
 * We call these Feynman Hellman c2pt baryon contractions.
 * This set of contractions are for different initial/final states (flavor changing).
 * Authors:
 * Arjun Gambhir
 * Do FH baryon contractions and write out the two-point correlator in hdf5 or asci
 */


#include "flavor_changing_fh_baryon_contractions_w.h"
#include "../contractions/proton_contractions_func_w.h"
#include "meas/inline/abs_inline_measurement_factory.h"
#include "../momentum/lalibe_sftmom.h"
#include "meas/inline/io/named_objmap.h"
#include "io/qprop_io.h"


namespace Chroma
{
  namespace LalibeFlavorChangingFHBaryonContractionsEnv
  {
    namespace
    {
      AbsInlineMeasurement* createMeasurement(XMLReader& xml_in,
					      const std::string& path)
      {
	return new InlineMeas(FlavorChangingFHBaryonParams(xml_in, path));
      }

      bool registered = false;
    }
    const std::string name = "FLAVOR_CHANGING_FH_BARYON_CONTRACTIONS";

    bool registerAll()
    {
      bool success = true;
      if (! registered)
      {
	success &= TheInlineMeasurementFactory::Instance().registerObject(name, createMeasurement);
	registered = true;
      }
      return success;
    }



    void read(XMLReader& xml, const std::string& path, FlavorChangingFHBaryonParams::Param_t& par)
    {
        XMLReader paramtop(xml, path);

        read(paramtop, "ng_parity", par.ng_parity);
        read(paramtop, "current", par.current);
        read(paramtop, "flavor", par.flavor);
#ifdef BUILD_HDF5
        read(paramtop, "h5_file_name", par.file_name);
        read(paramtop, "path", par.obj_path);
        QDPIO::cout<<"HDF5 found, writing to"<<par.file_name<<" to path "<<par.obj_path<<std::endl;
#endif
      //We set output_full_correlator to true if no momentum is specified.
      //read(paramtop, "output_full_correlator", par.output_full_correlator);
      if (paramtop.count("rotate_to_Dirac") != 0)
      {
	read(paramtop, "rotate_to_Dirac" ,par.rotate_to_Dirac);
	QDPIO::cout<<"Rotating to Dirac basis from DeGrand-Rossi basis option set to "<<par.rotate_to_Dirac<<std::endl;
      }
      else
      {
	QDPIO::cout<<"No XML option was specified regarding basis of propagators. Assuming DeGrand-Rossi and converting appropriately..."<<std::endl;
	par.rotate_to_Dirac = true;
      }
      if (paramtop.count("is_antiperiodic") != 0)
      {
	read(paramtop, "is_antiperiodic" ,par.is_antiperiodic);
	QDPIO::cout<<"Ther user says that antiperiodic flag is set to "<<par.is_antiperiodic<<std::endl;
      }
      else
      {
	QDPIO::cout<<"No XML option was specified regarding antiperiodicity. Assuming the quarks come from an antiperiodic lattice."<<std::endl;
	par.is_antiperiodic = true;
      }
      if (paramtop.count("p2_max") != 0)
      {
	read(paramtop, "p2_max" ,par.p2_max);
	par.output_full_correlator = false;
	QDPIO::cout<<"Reading momenta centered around the origin with a max of "<<par.p2_max<<std::endl;
	par.is_mom_max = true;
      }
      else if (paramtop.count("mom_list") != 0)
      {
	  read(paramtop, "mom_list" ,par.mom_list);
	  par.output_full_correlator = false;
	  QDPIO::cout<<"Using custom momentum list."<<std::endl;
	  par.is_mom_max = false;
	  //Assumes the length is 3 for the inner dimension.
	  for(int iter = 0; iter < par.mom_list.size(); iter++)
	    QDPIO::cout<<"Momentum px: "<<par.mom_list[iter][0]<<" py: "<<par.mom_list[iter][1]<<" pz: "<<par.mom_list[iter][2]<<std::endl;

	  //nested multi1d to multi2d
	  par.p_list.resize(par.mom_list.size(), Nd -1);
	  for(int mom = 0; mom < par.mom_list.size(); mom++)
	  {
	    par.p_list[mom][0] = par.mom_list[mom][0];
	    par.p_list[mom][1] = par.mom_list[mom][1];
	    par.p_list[mom][2] = par.mom_list[mom][2];
	  }
      }
      else
      {
	  QDPIO::cout << "No momentum specified, not doing any FTs and dumping full 4d-correlator."<<std::endl;
	  par.output_full_correlator = true;
	  //Below is only so SftMom does not crash...
	  par.is_mom_max = true;
	  par.p2_max = 0;
      }

      read(paramtop, "particle_list", par.particle_list);
    }


    void write(XMLWriter& xml, const std::string& path, const FlavorChangingFHBaryonParams::Param_t& par)
    {
      push(xml, path);

      write(xml, "ng_parity", par.ng_parity);
      write(xml, "current", par.current);
      write(xml, "flavor", par.flavor);
      write(xml, "rotate_to_Dirac", par.rotate_to_Dirac);
      write(xml, "is_antiperiodic", par.is_antiperiodic);
#ifdef BUILD_HDF5
      write(xml, "h5_file_name", par.file_name);
      write(xml, "path", par.obj_path);
#endif
      //write(xml, "output_full_correlator", par.output_full_correlator);
      if(par.is_mom_max == true)
	write(xml, "p2_max" ,par.p2_max);
      else
	write(xml, "mom_list" ,par.mom_list);
      write(xml, "particle_list", par.particle_list);

      pop(xml);
    }

    void read(XMLReader& xml, const std::string& path, FlavorChangingFHBaryonParams::NamedObject_t& input)
    {
      XMLReader inputtop(xml, path);

      //read(inputtop, "gauge_id" , input.gauge_id);
      //Logic to read quark propagators (whichever ones are present.)
      if (inputtop.count("up_quark") != 0)
      {
	read(inputtop, "up_quark" ,input.up_quark);
	QDPIO::cout<<"I found an up quark, here is its id: "<<input.up_quark<<std::endl;
	input.is_up = true;
      }
      else
      {
	  QDPIO::cout<<"I couldn't find an up quark, hope you don't need it for the inputted baryon contractions. "<<std::endl;
	  input.is_up = false;
      }
      if (inputtop.count("down_quark") != 0)
      {
	read(inputtop, "down_quark" ,input.down_quark);
	QDPIO::cout<<"I found a down quark, here is its id: "<<input.down_quark<<std::endl;
	input.is_down = true;
      }
      else
      {
	  QDPIO::cout<<"I couldn't find a down quark, hope you don't need it for the inputted baryon contractions. "<<std::endl;
	  input.is_down = false;
      }
      if (inputtop.count("strange_quark") != 0)
      {
	read(inputtop, "strange_quark" ,input.up_quark);
	QDPIO::cout<<"I found an strange quark, here is its id: "<<input.strange_quark<<std::endl;
	input.is_strange = true;
      }
      else
      {
	  QDPIO::cout<<"I couldn't find a strange quark, hope you don't need it for the inputted baryon contractions. "<<std::endl;
	  input.is_strange = false;
      }
      if (inputtop.count("charm_quark") != 0)
      {
	read(inputtop, "charm_quark" ,input.charm_quark);
	QDPIO::cout<<"I found an charm quark, here is its id: "<<input.charm_quark<<std::endl;
	input.is_charm = true;
      }
      else
      {
	  QDPIO::cout<<"I couldn't find a charm quark, hope you don't need it for the inputted baryon contractions. "<<std::endl;
	  input.is_charm = false;
      }
      read(inputtop, "fh_quark" ,input.fh_quark);
    }

    void write(XMLWriter& xml, const std::string& path, const FlavorChangingFHBaryonParams::NamedObject_t& input)
    {
      push(xml, path);
      //write(xml, "gauge_id" , input.gauge_id);
      if(input.is_up == true)
	write(xml, "up_quark" ,input.up_quark);
      if(input.is_down == true)
	write(xml, "down_quark" ,input.down_quark);
      if(input.is_strange == true)
	write(xml, "strange_quark" ,input.strange_quark);
      if(input.is_charm == true)
	write(xml, "charm_quark" ,input.charm_quark);
      write(xml, "fh_quark" ,input.fh_quark);
      pop(xml);
    }


    FlavorChangingFHBaryonParams::FlavorChangingFHBaryonParams()
    {
      frequency = 0;
    }

    FlavorChangingFHBaryonParams::FlavorChangingFHBaryonParams(XMLReader& xml_in, const std::string& path)
    {
      try
      {
	XMLReader paramtop(xml_in, path);

	if (paramtop.count("Frequency") == 1)
	  read(paramtop, "Frequency", frequency);
	else
	  frequency = 1;

	read(paramtop, "FlavorChangingFHBaryonParams", param);

	read(paramtop, "NamedObject", named_obj);

      }
      catch(const std::string& e)
      {
	QDPIO::cerr << __func__ << ": Caught Exception reading XML: " << e << std::endl;
	QDP_abort(1);
      }
    }


    void
    FlavorChangingFHBaryonParams::writeXML(XMLWriter& xml_out, const std::string& path)
    {
      push(xml_out, path);

      write(xml_out, "FlavorChangingFHBaryonParams", param);
      write(xml_out, "NamedObject", named_obj);

      pop(xml_out);
    }


    void  InlineMeas::operator()(unsigned long update_no, XMLWriter& xml_out)
    {
      START_CODE();

      StopWatch snoop;
      snoop.reset();
      snoop.start();

      QDPIO::cout<<"Baryon contractions starting..."<<std::endl;

      //Grab all the propagators that are given.
      XMLReader up_prop_file_xml, up_prop_record_xml;
      LatticePropagator up_quark_propagator;
      XMLReader down_prop_file_xml, down_prop_record_xml;
      LatticePropagator down_quark_propagator;
      XMLReader strange_prop_file_xml, strange_prop_record_xml;
      LatticePropagator strange_quark_propagator;
      XMLReader charm_prop_file_xml, charm_prop_record_xml;
      LatticePropagator charm_quark_propagator;
      //Add an fh propagator.
      XMLReader fh_prop_file_xml, fh_prop_record_xml;
      LatticePropagator fh_quark_propagator;
      //Need origin, j_decay, and t0 for fourier transform!
      //Need j_decay of bc to know what comes with a minus sign.
      int j_decay;
      int t_0;
      multi1d<int> origin;

      if(params.named_obj.is_up == true)
      {
	QDPIO::cout << "Attempting to read up propagator" << std::endl;
	try
	{
	    up_quark_propagator = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.up_quark);
	    TheNamedObjMap::Instance().get(params.named_obj.up_quark).getFileXML(up_prop_file_xml);
	    TheNamedObjMap::Instance().get(params.named_obj.up_quark).getRecordXML(up_prop_record_xml);
	    //Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
	    MakeSourceProp_t  orig_header;
	    if (up_prop_record_xml.count("/Propagator") != 0)
	    {
	      QDPIO::cout<<"Up quark propagator is unsmeared, reading from Propagator tag..."<<std::endl;
	      read(up_prop_record_xml, "/Propagator", orig_header);
	    }
	    else if (up_prop_record_xml.count("/SinkSmear") != 0)
	    {
	      QDPIO::cout<<"Up quark propagator is smeared, reading from SinkSmear tag..."<<std::endl;
	      read(up_prop_record_xml, "/SinkSmear", orig_header);
	    }
	    else
	    {
	      QDPIO::cout<<"What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..."<<std::endl;
	    }
	    j_decay = orig_header.source_header.j_decay;
	    t_0 = orig_header.source_header.t_source;
	    origin = orig_header.source_header.getTSrce();
	    //If we need to rotate, we do it now.
	    if(params.param.rotate_to_Dirac == true)
	      LegacyProton::rotate_to_Dirac_Basis(up_quark_propagator);
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

      if(params.named_obj.is_down == true)
      {
	QDPIO::cout << "Attempting to read down propagator" << std::endl;
	try
	{
	    down_quark_propagator = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.down_quark);
	    TheNamedObjMap::Instance().get(params.named_obj.down_quark).getFileXML(down_prop_file_xml);
	    TheNamedObjMap::Instance().get(params.named_obj.down_quark).getRecordXML(down_prop_record_xml);
	    //Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
	    MakeSourceProp_t  orig_header;
	    if (down_prop_record_xml.count("/Propagator") != 0)
	    {
	      QDPIO::cout<<"Down quark propagator is unsmeared, reading from Propagator tag..."<<std::endl;
	      read(down_prop_record_xml, "/Propagator", orig_header);
	    }
	    else if (down_prop_record_xml.count("/SinkSmear") != 0)
	    {
	      QDPIO::cout<<"Down quark propagator is smeared, reading from SinkSmear tag..."<<std::endl;
	      read(down_prop_record_xml, "/SinkSmear", orig_header);
	    }
	    else
	    {
	      QDPIO::cout<<"What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..."<<std::endl;
	    }
	    j_decay = orig_header.source_header.j_decay;
	    origin = orig_header.source_header.getTSrce();
	    //If we need to rotate, we do it now.
	    if(params.param.rotate_to_Dirac == true)
	      LegacyProton::rotate_to_Dirac_Basis(down_quark_propagator);
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

      if(params.named_obj.is_strange == true)
      {
	QDPIO::cout << "Attempting to read strange propagator" << std::endl;
	try
	{
	    strange_quark_propagator = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.strange_quark);
	    TheNamedObjMap::Instance().get(params.named_obj.strange_quark).getFileXML(strange_prop_file_xml);
	    TheNamedObjMap::Instance().get(params.named_obj.strange_quark).getRecordXML(strange_prop_record_xml);
	    //Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
	    MakeSourceProp_t  orig_header;
	    if (strange_prop_record_xml.count("/Propagator") != 0)
	    {
	      QDPIO::cout<<"Strange quark propagator is unsmeared, reading from Propagator tag..."<<std::endl;
	      read(strange_prop_record_xml, "/Propagator", orig_header);
	    }
	    else if (strange_prop_record_xml.count("/SinkSmear") != 0)
	    {
	      QDPIO::cout<<"Strange quark propagator is smeared, reading from SinkSmear tag..."<<std::endl;
	      read(strange_prop_record_xml, "/SinkSmear", orig_header);
	    }
	    else
	    {
	      QDPIO::cout<<"What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..."<<std::endl;
	    }
	    j_decay = orig_header.source_header.j_decay;
	    origin = orig_header.source_header.getTSrce();
	    //If we need to rotate, we do it now.
	    if(params.param.rotate_to_Dirac == true)
	      LegacyProton::rotate_to_Dirac_Basis(strange_quark_propagator);
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

      if(params.named_obj.is_charm == true)
      {
	QDPIO::cout << "Attempting to read charm propagator" << std::endl;
	try
	{
	    charm_quark_propagator = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.charm_quark);
	    TheNamedObjMap::Instance().get(params.named_obj.charm_quark).getFileXML(charm_prop_file_xml);
	    TheNamedObjMap::Instance().get(params.named_obj.charm_quark).getRecordXML(charm_prop_record_xml);
	    //Get the origin  and j_decay for the FT, this assumes all quarks have the same origin.
	    MakeSourceProp_t  orig_header;
	    if (charm_prop_record_xml.count("/Propagator") != 0)
	    {
	      QDPIO::cout<<"Charm quark propagator is unsmeared, reading from Propagator tag..."<<std::endl;
	      read(charm_prop_record_xml, "/Propagator", orig_header);
	    }
	    else if (charm_prop_record_xml.count("/SinkSmear") != 0)
	    {
	      QDPIO::cout<<"Charm quark propagator is smeared, reading from SinkSmear tag..."<<std::endl;
	      read(charm_prop_record_xml, "/SinkSmear", orig_header);
	    }
	    else
	    {
	      QDPIO::cout<<"What type of weird propagator did you give me? I can't find the right tag to get j_decay, source location, and whatnot...exiting..."<<std::endl;
	    }
	    j_decay = orig_header.source_header.j_decay;
	    origin = orig_header.source_header.getTSrce();
	    //If we need to rotate, we do it now.
	    if(params.param.rotate_to_Dirac == true)
	      LegacyProton::rotate_to_Dirac_Basis(charm_quark_propagator);
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

      //Here we read the fh prop.
      QDPIO::cout << "Attempting to read fh propagator" << std::endl;
      try
      {
	  fh_quark_propagator = TheNamedObjMap::Instance().getData<LatticePropagator>(params.named_obj.fh_quark);
	  TheNamedObjMap::Instance().get(params.named_obj.fh_quark).getFileXML(fh_prop_file_xml);
	  TheNamedObjMap::Instance().get(params.named_obj.fh_quark).getRecordXML(fh_prop_record_xml);
	  //If we need to rotate, we do it now.
	  if(params.param.rotate_to_Dirac == true)
	    LegacyProton::rotate_to_Dirac_Basis(fh_quark_propagator);
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

      //Initialize FT stuff here, whether this is used or not below is another story...
      LalibeSftMom ft = params.param.is_mom_max ? LalibeSftMom(params.param.p2_max, origin, false, j_decay)
	: LalibeSftMom(params.param.p_list, origin, j_decay);

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

      for(int particle_index = 0; particle_index < params.param.particle_list.size(); particle_index++)
      {
	if(params.param.particle_list[particle_index] == "proton_neutron")
	{
	  QDPIO::cout<<"Particle number "<<(particle_index+1)<<" is the proton_neutron."<<std::endl;
	  QDPIO::cout<<"Checking to make sure we have the correct quark propagators to compute the proton."<<std::endl;
	  if(params.named_obj.is_up == true && params.named_obj.is_down == true
	      && params.param.flavor == "UD")
	  {
	    QDPIO::cout<<"Found an up quark, down quark, and a Feyman Hellman up quark for the neutron to proton contraction. Starting calculation..."<<std::endl;
	    LatticeComplex proton = zero;
	    //The variable below is an accumulator for the different fh diagrams.
	    multi2d<int> snk_spins;
	    multi2d<int> src_spins;
	    multi1d<Real> snk_weights;
	    multi1d<Real> src_weights;
	    src_spins.resize(Ns, Nc);
	    snk_spins.resize(Ns, Nc);
	    src_weights.resize(Ns);
	    snk_weights.resize(Ns);
	    src_weights[0] = 1/sqrt(2); src_weights[1] = -1/sqrt(2); src_weights[2] = 1/sqrt(2); src_weights[3] = -1/sqrt(2);
	    snk_weights[0] = 1/sqrt(2); snk_weights[1] = -1/sqrt(2); snk_weights[2] = 1/sqrt(2); snk_weights[3] = -1/sqrt(2);
	    //spin up to spin up piece:
	    QDPIO::cout<<"Computing spin-up to spin-up piece..."<<std::endl;
	    //source spins:
	    src_spins[0][0] = 0; src_spins[0][1] = 1; src_spins[0][2] = 0;
	    src_spins[1][0] = 1; src_spins[1][1] = 0; src_spins[1][2] = 0;
	    src_spins[2][0] = 0; src_spins[2][1] = 0; src_spins[2][2] = 1;
	    src_spins[3][0] = 1; src_spins[3][1] = 0; src_spins[3][2] = 0;
	    //sink spins:
	    snk_spins[0][0] = 0; snk_spins[0][1] = 1; snk_spins[0][2] = 0;
	    snk_spins[1][0] = 1; snk_spins[1][1] = 0; snk_spins[1][2] = 0;
	    snk_spins[2][0] = 0; snk_spins[2][1] = 1; snk_spins[2][2] = 0;
	    snk_spins[3][0] = 0; snk_spins[3][1] = 0; snk_spins[3][2] = 1;
	    LegacyProton::spin_contraction(up_quark_propagator, down_quark_propagator, fh_quark_propagator, src_spins, snk_spins, src_weights, snk_weights, proton);
	    LegacyProton::write_correlator(params.param.output_full_correlator, params.param.is_antiperiodic,
		"fh_proton_neutron_"+params.param.current+"_"+params.param.flavor, "up",
#ifdef BUILD_HDF5
	        params.param.obj_path, h5out, wmode,
#endif
		t_0, Nt, origin, ft, proton);
	    QDPIO::cout<<"Computing spin-down to spin-down piece..."<<std::endl;
	    //spin down to spin down piece:
	    //source spins:
	    src_spins[0][0] = 0; src_spins[0][1] = 1; src_spins[0][2] = 1;
	    src_spins[1][0] = 1; src_spins[1][1] = 0; src_spins[1][2] = 1;
	    src_spins[2][0] = 0; src_spins[2][1] = 1; src_spins[2][2] = 1;
	    src_spins[3][0] = 1; src_spins[3][1] = 1; src_spins[3][2] = 0;
	    //sink spins:
	    snk_spins[0][0] = 0; snk_spins[0][1] = 1; snk_spins[0][2] = 1;
	    snk_spins[1][0] = 1; snk_spins[1][1] = 0; snk_spins[1][2] = 1;
	    snk_spins[2][0] = 1; snk_spins[2][1] = 1; snk_spins[2][2] = 0;
	    snk_spins[3][0] = 1; snk_spins[3][1] = 0; snk_spins[3][2] = 1;
	    LegacyProton::spin_contraction(up_quark_propagator, down_quark_propagator, fh_quark_propagator, src_spins, snk_spins, src_weights, snk_weights, proton);
	    LegacyProton::write_correlator(params.param.output_full_correlator, params.param.is_antiperiodic,
		"fh_proton_neutron_"+params.param.current+"_"+params.param.flavor, "dn",
#ifdef BUILD_HDF5
		params.param.obj_path, h5out, wmode,
#endif
		t_0, Nt, origin, ft, proton);
	    if(params.param.ng_parity == true)
	    {
	      //spin up to spin up piece:
	      QDPIO::cout<<"Now doing negative parity proton, because you asked me to..."<<std::endl;
	      QDPIO::cout<<"Computing spin-up to spin-up piece..."<<std::endl;
	      //source spins:
	      src_spins[0][0] = 2; src_spins[0][1] = 3; src_spins[0][2] = 2;
	      src_spins[1][0] = 3; src_spins[1][1] = 2; src_spins[1][2] = 2;
	      src_spins[2][0] = 2; src_spins[2][1] = 2; src_spins[2][2] = 3;
	      src_spins[3][0] = 3; src_spins[3][1] = 2; src_spins[3][2] = 2;
	      //sink spins:
	      snk_spins[0][0] = 2; snk_spins[0][1] = 3; snk_spins[0][2] = 2;
	      snk_spins[1][0] = 3; snk_spins[1][1] = 2; snk_spins[1][2] = 2;
	      snk_spins[2][0] = 2; snk_spins[2][1] = 3; snk_spins[2][2] = 2;
	      snk_spins[3][0] = 2; snk_spins[3][1] = 2; snk_spins[3][2] = 3;
	      LegacyProton::spin_contraction(up_quark_propagator, down_quark_propagator, fh_quark_propagator, src_spins, snk_spins, src_weights, snk_weights, proton);
	      LegacyProton::write_correlator(params.param.output_full_correlator, params.param.is_antiperiodic,
		  "fh_proton_neutron_np_"+params.param.current+"_"+params.param.flavor, "up",
#ifdef BUILD_HDF5
		  params.param.obj_path, h5out, wmode,
#endif
		  t_0, Nt, origin, ft, proton);
	      QDPIO::cout<<"Computing spin-down to spin-down piece..."<<std::endl;
	      //source spins:
	      src_spins[0][0] = 2; src_spins[0][1] = 3; src_spins[0][2] = 3;
	      src_spins[1][0] = 3; src_spins[1][1] = 2; src_spins[1][2] = 3;
	      src_spins[2][0] = 2; src_spins[2][1] = 3; src_spins[2][2] = 3;
	      src_spins[3][0] = 3; src_spins[3][1] = 3; src_spins[3][2] = 2;
	      //sink spins:
	      snk_spins[0][0] = 2; snk_spins[0][1] = 3; snk_spins[0][2] = 3;
	      snk_spins[1][0] = 3; snk_spins[1][1] = 2; snk_spins[1][2] = 3;
	      snk_spins[2][0] = 3; snk_spins[2][1] = 3; snk_spins[2][2] = 2;
	      snk_spins[3][0] = 3; snk_spins[3][1] = 2; snk_spins[3][2] = 3;
	      LegacyProton::spin_contraction(up_quark_propagator, down_quark_propagator, fh_quark_propagator, src_spins, snk_spins, src_weights, snk_weights, proton);
	      LegacyProton::write_correlator(params.param.output_full_correlator, params.param.is_antiperiodic,
		  "fh_proton_neutron_np_"+params.param.current+"_"+params.param.flavor, "dn",
#ifdef BUILD_HDF5
		  params.param.obj_path, h5out, wmode,
#endif
		  t_0, Nt, origin, ft, proton);
	    }
	  }
	  else
	    QDPIO::cout<<"Sorry, I couldn't find both an up quark and down quark. Skipping the proton contraction..."<<std::endl;
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
