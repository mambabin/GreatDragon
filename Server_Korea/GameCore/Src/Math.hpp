#ifndef _MATH_HPP_
#define _MATH_HPP_

#include "Math.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class Vector2i {

	public:
		Vector2i() {
			x_ = 0;
			y_ = 0;
		}

	public:
		void ToPB(PB_Vector2i *pb_) const {
			pb_->Clear();
			pb_->set_x(x_);
			pb_->set_y(y_);
		}
		void FromPB(const PB_Vector2i *pb_) {
			x_ = pb_->x();
			y_ = pb_->y();
		}

	public:
		inline int32_t x() const {
			return x_;
		}
		inline void set_x(int32_t value) {
			x_ = value;
		}

		inline int32_t y() const {
			return y_;
		}
		inline void set_y(int32_t value) {
			y_ = value;
		}

	private:
		int32_t x_;

		int32_t y_;

};

class Vector3f {

	public:
		Vector3f() {
			x_ = 0.0f;
			y_ = 0.0f;
			z_ = 0.0f;
		}

	public:
		void ToPB(PB_Vector3f *pb_) const {
			pb_->Clear();
			pb_->set_x(x_);
			pb_->set_y(y_);
			pb_->set_z(z_);
		}
		void FromPB(const PB_Vector3f *pb_) {
			x_ = pb_->x();
			y_ = pb_->y();
			z_ = pb_->z();
		}

	public:
		inline float x() const {
			return x_;
		}
		inline void set_x(float value) {
			x_ = value;
		}

		inline float y() const {
			return y_;
		}
		inline void set_y(float value) {
			y_ = value;
		}

		inline float z() const {
			return z_;
		}
		inline void set_z(float value) {
			z_ = value;
		}

	private:
		float x_;

		float y_;

		float z_;

};

class Vector4f {

	public:
		Vector4f() {
			x_ = 0.0f;
			y_ = 0.0f;
			z_ = 0.0f;
			w_ = 0.0f;
		}

	public:
		void ToPB(PB_Vector4f *pb_) const {
			pb_->Clear();
			pb_->set_x(x_);
			pb_->set_y(y_);
			pb_->set_z(z_);
			pb_->set_w(w_);
		}
		void FromPB(const PB_Vector4f *pb_) {
			x_ = pb_->x();
			y_ = pb_->y();
			z_ = pb_->z();
			w_ = pb_->w();
		}

	public:
		inline float x() const {
			return x_;
		}
		inline void set_x(float value) {
			x_ = value;
		}

		inline float y() const {
			return y_;
		}
		inline void set_y(float value) {
			y_ = value;
		}

		inline float z() const {
			return z_;
		}
		inline void set_z(float value) {
			z_ = value;
		}

		inline float w() const {
			return w_;
		}
		inline void set_w(float value) {
			w_ = value;
		}

	private:
		float x_;

		float y_;

		float z_;

		float w_;

};

#endif
