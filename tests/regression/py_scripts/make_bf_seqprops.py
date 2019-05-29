import sys
import h5py as h5
import tables
import numpy as np
import gamma
import time
import contractions
np.set_printoptions(linewidth=180)

''' This code reads a single h5 file that stores a propagator from a point source
    to all, from all locations on the lattice.  It uses these props to manually
    construct the 'sequential' propagator

    seqprop[t,zz,yy,xx,i,j,a,b] = curr_prop[t,zz,yy,xx,i,k,a,c] G[k,kk] prop[1,z,y,x,kk,j,c,b]

    and then construct the 3pt function.  It compares these 'brute force' 3pt
    functions to the ones generated by the LALIBE code.

    Currently tested and passing
        A3 - 0 momentum at the sink and current, proton spin up and dn, full and UPPER/LOWER seqprops
            DD positive parity including boundary wrapping
            DD negative parity including boundary wrapping
            UU positive parity including boundary wrapping
            UU negative parity including boundary wrapping
'''
quark_spin='half'
spin='up'


f = h5.File('test_lalibe/all_pt_props.h5','r')
f_seqprop = tables.open_file('test_lalibe/seqprop_bf.h5','a')
if 'props' not in f_seqprop.root:
    f_seqprop.create_group('/','props')

g_a3 = np.einsum('ik,kj->ij',gamma.g_3,gamma.g_5)

''' set source at x=y=z=0 '''
t_src='3'
src='x0y0z0t'+t_src
prop = f['props/pt_prop_x0_y0_z0_t'+t_src][()]

for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    if seqprop_name not in f_seqprop.get_node('/props'):
        start_time = time.time()
        seqprop = np.zeros_like(prop)
        for x in range(4):
            for y in range(4):
                for z in range(4):
                    curr_prop = f['props/pt_prop_x%d_y%d_z%d_t%d' %(x,y,z,t)][()]
                    '''
                    seqprop[t,zz,yy,xx,i,j,a,b] = curr_prop[t,zz,yy,xx,i,k,a,c] G[k,kk] prop[1,z,y,x,kk,j,c,b]
                    '''
                    # multiply by Gamma
                    curr_prop = np.einsum('tzyxikab,kj->tzyxijab',curr_prop,g_a3)
                    seqprop  += np.einsum('tzyxikac,kjcb->tzyxijab',curr_prop,prop[t,z,y,x])
        f_seqprop.create_array('/props',seqprop_name,seqprop)
        stop_time = time.time()
        print('t = %d, seconds = %.2f' %(t,stop_time-start_time))
    else:
        print('t = %d, already created' %t)

''' set source at x=y=z=0 '''
t_src='6'
src='x0y0z0t'+t_src
prop = f['props/pt_prop_x0_y0_z0_t'+t_src][()]

for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    if seqprop_name not in f_seqprop.get_node('/props'):
        start_time = time.time()
        seqprop = np.zeros_like(prop)
        for x in range(4):
            for y in range(4):
                for z in range(4):
                    curr_prop = f['props/pt_prop_x%d_y%d_z%d_t%d' %(x,y,z,t)][()]
                    '''
                    seqprop[t,zz,yy,xx,i,j,a,b] = curr_prop[t,zz,yy,xx,i,k,a,c] G[k,kk] prop[1,z,y,x,kk,j,c,b]
                    '''
                    # multiply by Gamma
                    curr_prop = np.einsum('tzyxikab,kj->tzyxijab',curr_prop,g_a3)
                    seqprop  += np.einsum('tzyxikac,kjcb->tzyxijab',curr_prop,prop[t,z,y,x])
        f_seqprop.create_array('/props',seqprop_name,seqprop)
        stop_time = time.time()
        print('t = %d, seconds = %.2f' %(t,stop_time-start_time))
    else:
        print('t = %d, already created' %t)
f_seqprop.close()

''' perform contractions '''
U    = gamma.U_DR_to_DP
Uadj = gamma.U_DR_to_DP_adj

t_src='3'
src='x0y0z0t'+t_src
t_sep=4
t_sink = (int(t_src) + t_sep) % 8

prop = f['props/pt_prop_x0_y0_z0_t'+t_src][()]
prop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,prop,U)
f_seqprop = tables.open_file('test_lalibe/seqprop_bf.h5','r')

all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' DD '''
    proton = contractions.proton_spin_contract(prop_DP,prop_DP,seqprop_DP,'proton',spin)
    proton_time = np.einsum('tzyx->t',proton)
    #print('tg = %d' %t)
    #print('real', proton_time.real)
    #print('imag', proton_time.imag)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src)+t_sep >= 8:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_DD_%s_%s_t0_3_tsep_4_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src)).read()
lalibe_3pt.close()

have_old=True
try:
    lalibe_3pt_old = tables.open_file('test_lalibe/lalibe_3ptfn.old.h5','r')
    a3_old = lalibe_3pt_old.get_node('/PP_seqprop_proton_DD_%s_%s_tsrc_3_tsep_4/A3/x0_y0_z0_t%s/px0_py0_pz0/corr_local_fn' %(spin,spin,t_src)).read()
    lalibe_3pt_old.close()
except:
    print('DD %s not run in old code yet' %(spin))
    have_old=False

np.set_printoptions(precision=6)
print('\nA3 DD %s corr, src=0,0,0,%s, t_sep=4\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
if have_old:
    print(a3_old.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)
if have_old:
    print(a3_old.imag)

''' UU up '''
all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' spin up, DD '''
    proton  = contractions.proton_spin_contract(seqprop_DP,prop_DP,prop_DP,'proton',spin)
    proton += contractions.proton_spin_contract(prop_DP,seqprop_DP,prop_DP,'proton',spin)
    proton_time = np.einsum('tzyx->t',proton)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src)+t_sep >= 8:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_UU_%s_%s_t0_%s_tsep_4_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src,t_src)).read()
lalibe_3pt.close()

""" NEED to run this
lalibe_3pt_old = tables.open_file('test_lalibe/lalibe_3ptfn.old.h5','r')
a3_old = lalibe_3pt_old.get_node('/PP_seqprop_proton_UU_up_up_tsrc_3_tsep_4/A3/x0_y0_z0_t%s/px0_py0_pz0/corr_local_fn' %t_src).read()
lalibe_3pt_old.close()
"""

np.set_printoptions(precision=6)
print('\nA3 UU %s corr, src=0,0,0,%s, t_sep=4\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
#print(a3_old.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)
#print(a3_old.imag)


''' proton, DD, up, t=6 '''

t_src='6'
src='x0y0z0t'+t_src
t_sink = (int(t_src) + t_sep) % 8

prop = f['props/pt_prop_x0_y0_z0_t'+t_src][()]
prop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,prop,U)
p_2pt_up = contractions.proton_spin_contract(prop_DP,prop_DP,prop_DP,'proton',spin)
p_2pt_up_time = np.einsum('tzyx->t',p_2pt_up)
print('\nProton 2pt')
print(p_2pt_up_time.real)
all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' spin up, DD '''
    proton = contractions.proton_spin_contract(prop_DP,prop_DP,seqprop_DP,'proton',spin)
    proton_time = np.einsum('tzyx->t',proton)
    #print('tg = %d' %t)
    #print('real', proton_time.real)
    #print('imag', proton_time.imag)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src)+t_sep >= 8:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_DD_%s_%s_t0_%s_tsep_4_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src,t_src)).read()
lalibe_3pt.close()

have_old=True
try:
    lalibe_3pt_old = tables.open_file('test_lalibe/lalibe_3ptfn.old.h5','r')
    a3_old = lalibe_3pt_old.get_node('/PP_seqprop_proton_DD_%s_%s_tsrc_%s_tsep_4/A3/x0_y0_z0_t%s/px0_py0_pz0/corr_local_fn' %(spin,spin,t_src,t_src)).read()
    lalibe_3pt_old.close()
except:
    have_old=False
    print('DD %s not run in old code yet' %(spin))

np.set_printoptions(precision=6)
print('\nA3 DD %s corr, src=0,0,0,%s, t_sep=4\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
if have_old:
    print(a3_old.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)
if have_old:
    print(a3_old.imag)

''' UU up '''
all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' spin up, DD '''
    proton  = contractions.proton_spin_contract(seqprop_DP,prop_DP,prop_DP,'proton',spin)
    proton += contractions.proton_spin_contract(prop_DP,seqprop_DP,prop_DP,'proton',spin)
    proton_time = np.einsum('tzyx->t',proton)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src)+t_sep >= 8:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_UU_%s_%s_t0_%s_tsep_4_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src,t_src)).read()
lalibe_3pt.close()

""" NEED to run this
lalibe_3pt_old = tables.open_file('test_lalibe/lalibe_3ptfn.old.h5','r')
a3_old = lalibe_3pt_old.get_node('/PP_seqprop_proton_UU_up_up_tsrc_3_tsep_4/A3/x0_y0_z0_t%s/px0_py0_pz0/corr_local_fn' %t_src).read()
lalibe_3pt_old.close()
"""

np.set_printoptions(precision=6)
print('\nA3 UU %s corr, src=0,0,0,%s, t_sep=4\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
#print(a3_old.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)
#print(a3_old.imag)


''' negative parity, no boundary wrap '''
t_src='6'
t_sep=-4
src='x0y0z0t'+t_src
t_sink = (int(t_src) + t_sep) % 8

prop = f['props/pt_prop_x0_y0_z0_t'+t_src][()]
prop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,prop,U)

all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' spin up, DD '''
    proton = contractions.proton_spin_contract(prop_DP,prop_DP,seqprop_DP,'proton_np',spin)
    proton_time = np.einsum('tzyx->t',proton)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src) + t_sep < 0:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_np_DD_%s_%s_t0_%s_tsep_%s_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src,str(t_sep),t_src)).read()
lalibe_3pt.close()

'''
lalibe_3pt_old = tables.open_file('test_lalibe/lalibe_3ptfn.old.h5','r')
a3_old = lalibe_3pt_old.get_node('/PP_seqprop_proton_DD_up_up_tsrc_3_tsep_4/A3/x0_y0_z0_t3/px0_py0_pz0/corr_local_fn').read()
lalibe_3pt_old.close()
'''

np.set_printoptions(precision=6)
print('\nA3 NEG PAR DD %s corr, src=0,0,0,%s, t_sep=%s\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src,str(t_sep)))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
#print(a3_old.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)
#print(a3_old.imag)

''' UU up NP '''
all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' spin up, DD '''
    proton  = contractions.proton_spin_contract(seqprop_DP,prop_DP,prop_DP,'proton_np',spin)
    proton += contractions.proton_spin_contract(prop_DP,seqprop_DP,prop_DP,'proton_np',spin)
    proton_time = np.einsum('tzyx->t',proton)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src) + t_sep < 0:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_np_UU_%s_%s_t0_%s_tsep_%s_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src,str(t_sep),t_src)).read()
lalibe_3pt.close()

np.set_printoptions(precision=6)
print('\nA3 NEG PAR UU %s corr, src=0,0,0,%s, t_sep=%s\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src,str(t_sep)))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)




''' negative parity, boundary wrap '''
t_src='3'
t_sep=-4
src='x0y0z0t'+t_src
t_sink = (int(t_src) + t_sep) % 8

prop = f['props/pt_prop_x0_y0_z0_t'+t_src][()]
prop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,prop,U)

all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' spin up, DD '''
    proton = contractions.proton_spin_contract(prop_DP,prop_DP,seqprop_DP,'proton_np',spin)
    proton_time = np.einsum('tzyx->t',proton)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src) + t_sep < 0:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_np_DD_%s_%s_t0_%s_tsep_%s_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src,str(t_sep),t_src)).read()
lalibe_3pt.close()

'''
lalibe_3pt_old = tables.open_file('test_lalibe/lalibe_3ptfn.old.h5','r')
a3_old = lalibe_3pt_old.get_node('/PP_seqprop_proton_DD_up_up_tsrc_3_tsep_4/A3/x0_y0_z0_t3/px0_py0_pz0/corr_local_fn').read()
lalibe_3pt_old.close()
'''

np.set_printoptions(precision=6)
print('\nA3 NEG PAR DD %s corr, src=0,0,0,%s, t_sep=%s\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src,str(t_sep)))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
#print(a3_old.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)
#print(a3_old.imag)

''' UU up NP '''
all_proton_corrs = []
for t in range(8):
    seqprop_name = 'seqprop_src'+src+'_tg'+str(t)
    seqprop = f_seqprop.get_node('/props/'+seqprop_name).read()
    seqprop_DP = np.einsum('ik,tzyxklab,lj->tzyxijab',Uadj,seqprop,U)
    ''' spin up, DD '''
    proton  = contractions.proton_spin_contract(seqprop_DP,prop_DP,prop_DP,'proton_np',spin)
    proton += contractions.proton_spin_contract(prop_DP,seqprop_DP,prop_DP,'proton_np',spin)
    proton_time = np.einsum('tzyx->t',proton)
    all_proton_corrs.append(proton_time)
all_proton_corrs = np.array(all_proton_corrs)
all_proton_corrs = np.roll(all_proton_corrs,-int(t_src),axis=0)
if int(t_src) + t_sep < 0:
    all_proton_corrs = -all_proton_corrs

lalibe_3pt = tables.open_file('test_lalibe/lalibe_3ptfn.h5','r')
a3 = lalibe_3pt.get_node('/'+quark_spin+'/proton_np_UU_%s_%s_t0_%s_tsep_%s_sink_mom_px0_py0_pz0/A3/x0_y0_z0_t%s/px0_py0_pz0/local_current' %(spin,spin,t_src,str(t_sep),t_src)).read()
lalibe_3pt.close()

np.set_printoptions(precision=6)
print('\nA3 NEG PAR UU %s corr, src=0,0,0,%s, t_sep=%s\nBrute Force\nLALIBE\nOLD LALIBE' %(spin,t_src,str(t_sep)))
print('real')
print(all_proton_corrs[:,t_sink].real)
print(a3.real)
print('imag')
print(all_proton_corrs[:,t_sink].imag)
print(a3.imag)


f.close()
f_seqprop.close()