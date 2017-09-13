//
// Created by Gurgen Hayrapetyan on 9/9/17.
//

#include "OrbitTransfer.h"
#include <iostream>
#include "BVPSolver.h"
#include "CollocationSolver.h"

OrbitTransfer::OrbitTransfer() : DifferentialSystem(0.0, 1.0, 7) {

}

OrbitTransfer::OrbitTransfer(double mu, double m0, double Isp, double T, double r0, double tf) :  DifferentialSystem(0.0, 1.0, 7) {
    this->mu = mu;
    this->m0 = m0;
    this->Isp = Isp;
    this->T = T;
    this->r0 = r0;
    this->tf = tf;
    this->v0 = sqrt(mu/r0); // initial tangential velocity of circular orbit
    this->Tbar = T*tf/(v0*1000); // nondimensionalized thrust
    this->mdot = T/Isp/9.80665; // mass flow rate
    this->eta = v0*tf/r0; // scaling
}

Eigen::VectorXd OrbitTransfer::RhsFunc(double t, const Eigen::VectorXd& y)
{
    Eigen::VectorXd rs = Eigen::VectorXd(7);
    double r = y(0);
    double u = y(1);
    double v = y(2);

    double pr = y(4);
    double pu = y(5);
    double pv = y(6);

    double rsq = r*r;

    double post = (Tbar/(m0-mdot*t*tf))/(sqrt(pu*pu+pv*pv));

    rs << u*eta,
            (v*v/r-1/rsq)*eta+post*pu,
            -u*v*eta/r + post*pv,
            v*eta/r,
            pu*(v*v/rsq-2/(rsq*r))*eta-pv*u*v*eta/rsq,
            -pr*eta+pv*v/r,
            -pu*2*v*eta/r+pv*u*eta/r;
    return rs;
}

Eigen::MatrixXd OrbitTransfer::RhsGradYFunc(double t, const Eigen::VectorXd& y)
{
    double r = y(0);
    double u = y(1);
    double v = y(2);

    double pu = y(5);
    double pv = y(6);

    double rsq = r*r;

    double post = (Tbar/(m0-mdot*t*tf))/(sqrt(pu*pu+pv*pv));

    double pupvsq = pu*pu+pv*pv;
    double prdr = pu*(-2*v*v/(rsq*r)+6/(rsq*rsq))*eta +2*pv*u*v*eta/(rsq*r);
    double prdu = -pv*v*eta/rsq;
    double prdv =  2*pu*v*eta/rsq-pv*u*eta/rsq;

    Eigen::MatrixXd rs = Eigen::MatrixXd(7,7);
    rs << 0,eta,0,0,0,0,0,
            eta*(-v*v/rsq+2/(rsq*r)), 0, 2*v*eta/r, 0, 0, post-post*pu*pu/pupvsq, -post*pu*pv/pupvsq,
            u*v*eta/rsq, -v*eta/r, -u*eta/r, 0, 0, -post*pu*pv/pupvsq, post-post*pv*pv/pupvsq,
            -v*eta/rsq, 0, eta/r, 0,0,0,0,
            prdr, prdu, prdv, 0, 0, (v*v/rsq-2/(rsq*r))*eta, -u*v*eta/rsq,
            -pv*v/rsq, 0, pv/r, 0, -eta, 0, v/r,
            pu*2*v*eta/rsq-pv*u*eta/rsq, pv*eta/r, -pu*2*eta/r, 0, 0, -2*v*eta/r, u*eta/r;


    return rs;
}
Eigen::VectorXd OrbitTransfer::BcsFunc(const Eigen::VectorXd& y1, const Eigen::VectorXd& y2)
{
    double r = y1(0);
    double u = y1(1);
    double v = y1(2);
    double theta = y1(3);

    double r2 = y2(0);
    double u2 = y2(1);
    double v2 = y2(2);
    double pr2 = y2(4);

    double pv2 = y2(6);


    Eigen::VectorXd rs = Eigen::VectorXd(7);
    rs << r-1, u, v-1, theta, u2, v2-sqrt(1/r2), -1-pr2+0.5*pv2*sqrt(1/(r2*r2*r2));
    return rs;
}

Eigen::MatrixXd OrbitTransfer::BcsGrad1Func(const Eigen::VectorXd& y1, const Eigen::VectorXd& y2)
{
    Eigen::MatrixXd rs = Eigen::MatrixXd(7,7);
    rs <<   1,0,0,0,0,0,0,
            0,1,0,0,0,0,0,
            0,0,1,0,0,0,0,
            0,0,0,1,0,0,0,
            0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,
            0,0,0,0,0,0,0;

    return rs;
}

Eigen::MatrixXd OrbitTransfer::BcsGrad2Func(const Eigen::VectorXd& y1, const Eigen::VectorXd& y2)
{
    double r2 = y2(0);
    double pv2 = y2(6);

    Eigen::MatrixXd rs = Eigen::MatrixXd(7,7);

    rs <<   0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,
            0,1,0,0,0,0,0,
            0.5*pow(r2,-1.5),0,1,0,0,0,0,
            -(3.0/4.0)*pv2*pow(r2,-2.5),0,0,0,-1,0,0.5*sqrt(1/(r2*r2*r2));
    return rs;
}

void OrbitTransfer::run() {
    std::cout << "Hello, World!" << std::endl;

    std::cout << "Setting up DS" << std::endl;

    std::cout << "Setting up BVP" << std::endl;

    int N = 1000;
    Eigen::VectorXd init_guess = Eigen::VectorXd::Zero((N+1)*dim());

    for (int i=0; i<N+1; i++) {
        init_guess(i * dim_) = 1;
        init_guess(i * dim_ + 2) = 1;
        init_guess(i * dim_ + 5) = -1;
    }

    // Instantiate a CollocationSolver with 1000 grid points for a system of
    CollocationSolver bvp(this, N, init_guess);
    // collocation parameters
    Eigen::MatrixXd a = Eigen::MatrixXd(3,3);
    Eigen::VectorXd rho = Eigen::VectorXd(3);
    Eigen::VectorXd b = Eigen::VectorXd(3);
    int k = 3;
    rho << 0.0, 0.5, 1.0;
    b << 1.0/6.0, 2.0/3.0, 1.0/6.0;
    a << 0, 0, 0,
            5.0/24.0, 1.0/3.0, -1.0/24.0,
            1.0/6.0, 2.0/3.0, 1.0/6.0;
    bvp.SetScheme(k, rho, a, b);

    std::cout << "Solving..." << std::endl;
    for (int i=1; i<11; i++) {
        tf = 0.1*i*24*3600; // final time (s)
        Tbar = T*tf/(v0*1000); // nondimensionalized thrust
        eta = v0*tf/r0; // scaling
        std::cout << "Solving for tf=" << 0.1 * i << "days" << std::endl;
        int status = bvp.Solve();
        if (status == 0) {
            std::cout << "Solved for tf=" << 0.1 * i << "days" << std::endl;
        } else {
            std::cout << "Iteration limit exceeded." << std::endl;
            return;
        }
    }
    for (int i=0; i < bvp.sol_vec().size()/2; i++)
    {
        std::cout << bvp.Step(i) << " " << bvp.sol_vec()(2*i) << std::endl;
    }
    bvp.WriteSolutionFile();
}

//int lambert(boost::python::list& r1, boost::python::list& r2, double dt, bool prograde, const double mu)
//{
//    r.resize(2);
//    r[0] << boost::python::extract<double>(r1[0]), boost::python::extract<double>(r1[1]),
//            boost::python::extract<double>(r1[2]);
//    r[1] << boost::python::extract<double>(r2[0]), boost::python::extract<double>(r2[1]),
//            boost::python::extract<double>(r2[2]);
//    orbit_desc(r_res, v_res, r[0], r[1], dt, prograde, mu);
//    // std::cout << r_res << std::endl;
//    // std::cout << v_res << std::endl;
//    return 1;
//}
//
//boost::python::list get_v0()
//{
//    boost::python::list list;
//    list.append(v_res(0));
//    list.append(v_res(1));
//    list.append(v_res(2));
//    return list;
//}

char const* OrbitTransfer::greet()
{
    return "hello, world";
}

#include <boost/python.hpp>
//#include <boost/python/numpy.hpp>

using namespace boost::python;

BOOST_PYTHON_MODULE(OrbitTransfer)
{
    // class_<std::vector<double> >("double_vector")
    //        .def(vector_indexing_suite<std::vector<double> >())
    //    ;
    //void (omt::*orbit_desc)(A&) = &Foo::orbit_desc;
    class_<OrbitTransfer>("OrbitTransfer")
//            .def("greet", &OrbitTransfer::greet)
            .def("run", &OrbitTransfer::run)
            ;
}