/*
 * Sliding_mode_4dof_comp.hpp
 *
 *  Created on: 18-Mar-2015
 *      Author: nilxwam
 */

#ifndef SLIDING_MODE_4DOF_COMP_HPP_
#define SLIDING_MODE_4DOF_COMP_HPP_

#include <barrett/math/traits.h>
#include <list>
#include <barrett/units.h>
#include <barrett/detail/ca_macro.h>
#include <barrett/systems/abstract/system.h>

#include <eigen3/Eigen/Core>
#include <libconfig.h++>

#include <barrett/detail/ca_macro.h>
#include <barrett/math/traits.h>
#include <barrett/systems/abstract/execution_manager.h>
#include <barrett/systems/abstract/controller.h>
#include <samlibs.h>

#include <math.h>

using namespace barrett;
using namespace systems;

template<size_t DOF>
class Slidingmode_Controller: public System {

	BARRETT_UNITS_TEMPLATE_TYPEDEFS(DOF);

	/*The sliding mode controller requires the following inputs
	 * Reference Jp, Jv, Ja
	 * Actual System Jp, Jv for feedback
	 * M matrix
	 * C matrix
	 * G matrix (optional for now)
	 * */

	/*Output of this system is
	 * Torque*/
public:
//	Input<bool> Status;
	Input<jp_type> referencejpInput;
	Input<jv_type> referencejvInput;
	Input<ja_type> referencejaInput;
public:
	Input<jp_type> feedbackjpInput;
	Input<jv_type> feedbackjvInput;
public:
	Input<Eigen::Matrix4d> M;
	Input<Eigen::Vector4d> C;
	Input<jt_type> C_comp;
public:
	Output<jt_type> controlOutput;

protected:
	typename Output<jt_type>::Value* controlOutputValue;

public:
	Slidingmode_Controller(/*systems::ExecutionManager* em*/bool status,
			const Eigen::Matrix4d lamda, const double coeff, const double delta,
			const std::string& sysName = "Slidingmode_Controller") :
			System(sysName), referencejpInput(this), referencejvInput(this), referencejaInput(
					this), feedbackjpInput(this), feedbackjvInput(this), M(
					this), C(this), C_comp(this),

			controlOutput(this, &controlOutputValue), STATUS(status), Lamda(
					lamda), Coeff(coeff), Delta(delta) {
//		if (em != NULL){
//		      em->startManaging(*this);
//		    }
	}
	virtual ~Slidingmode_Controller() {
		this->mandatoryCleanUp();
	}

protected:
	bool STATUS;
	Eigen::Matrix4d Lamda;
	double Coeff;
	double Delta;
	Eigen::Matrix4d M_inside;
	Eigen::Vector4d C_inside;
	Eigen::Vector4d C_inside_comp_tmp;
	jt_type C_inside_comp;

	jt_type jt_out;
	jp_type jp_sys, jp_ref, ep;
	jv_type jv_sys, jv_ref, ev;
	ja_type ja_ref;

	Eigen::Vector4d S, tmp_p, tmp_v, tmp_ev, tmp_ep, tmp_control, tmp_aref,
			tmp_pref, tmp_vref, jt_out_tmp;

	virtual void operate() {
		/*Taking reference values from the input terminal of this system*/
		jp_ref = this->referencejpInput.getValue();
		jv_ref = this->referencejvInput.getValue();
		ja_ref = this->referencejaInput.getValue();
		/*Taking feedback values from the input terminal of this system*/
		jp_sys = this->feedbackjpInput.getValue();
		jv_sys = this->feedbackjvInput.getValue();
		/*Taking M and C values from the input terminal of this system*/
		M_inside = this->M.getValue();
		C_inside = this->C.getValue();
		C_inside_comp = this->C_comp.getValue();

		ep = jp_sys - jp_ref;
		ev = jv_sys - jv_ref;

		tmp_p << jp_sys[0], jp_sys[1], jp_sys[2], jp_sys[3];
		tmp_v << jv_sys[0], jv_sys[1], jv_sys[2], jv_sys[3];
		tmp_aref << ja_ref[0], ja_ref[1], ja_ref[2], ja_ref[3];
		tmp_vref << jv_ref[0], jv_ref[1], jv_ref[2], jv_ref[3];
		tmp_pref << jp_ref[0], jp_ref[1], jp_ref[2], jp_ref[3];
		tmp_ev << ev[0], ev[1], ev[2], ev[3];
		tmp_ep << ep[0], ep[1], ep[2], ep[3];

		S = tmp_ev + Lamda * tmp_ep;

		tmp_control[0] = S[0] / (fabs(S[0]) + Delta);
		tmp_control[1] = S[1] / (fabs(S[1]) + Delta);
		tmp_control[2] = S[2] / (fabs(S[2]) + Delta);
		tmp_control[3] = S[3] / (fabs(S[3]) + Delta);

		C_inside_comp_tmp = Eigen::Vector4d::Zero();
		//	C_inside_comp_tmp[0] = C_inside_comp[0];
		if (abs(C_inside_comp[1]) <= 4) {
			C_inside_comp_tmp[1] = C_inside_comp[1];
		}
		if (isnan(C_inside_comp[1]) == true) {
					C_inside_comp_tmp[1] = 0;
				}
		//	C_inside_comp_tmp[2] = C_inside_comp[2];

		//	C_inside_comp_tmp[3] = C_inside_comp[3];
		jt_out_tmp = Eigen::Vector4d::Zero();

		if (STATUS == true) {

			jt_out_tmp = C_inside + C_inside_comp_tmp
					+ M_inside
							* (tmp_aref - Lamda * (tmp_v - tmp_vref)
									- Coeff * tmp_control);
//			if (jt_out_tmp[0] >= 10.5) {
//				jt_out_tmp[0] = 10.5;
//			}
//
//			if (jt_out_tmp[1] >= 10.5) {
//				jt_out_tmp[1] = 10.5;
//			}
//
//			if (jt_out_tmp[2] >= 10.5) {
//				jt_out_tmp[2] = 10.5;
//			}
//
//			if (jt_out_tmp[3] >= 10.5) {
//				jt_out_tmp[3] = 10.5;
//			}
//
		} else {
			jt_out_tmp = C_inside
					+ M_inside
							* (tmp_aref - Lamda * (tmp_v - tmp_vref)
									- Coeff  * tmp_control);
		}

		jt_out[0] = jt_out_tmp[0];
		jt_out[1] = jt_out_tmp[1];
		jt_out[2] = jt_out_tmp[2];
		jt_out[3] = jt_out_tmp[3];

		controlOutputValue->setData(&jt_out);
	}

private:
	DISALLOW_COPY_AND_ASSIGN(Slidingmode_Controller);
};

#endif /* SLIDING_MODE_4DOF_COMP_HPP_ */
