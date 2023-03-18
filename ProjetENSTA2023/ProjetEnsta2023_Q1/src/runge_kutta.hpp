#ifndef _NUMERIC_RUNGE_KUTTA_HPP_
#define _NUMERIC_RUNGE_KUTTA_HPP_
#include <utility>
#include "vortex.hpp"
#include "cloud_of_points.hpp"
#include "cartesian_grid_of_speed.hpp"

namespace Numeric 
{

    Geometry::CloudOfPoints
    solve_RK4_fixed_vortices( double dt, CartesianGridOfSpeed const& speed, 
                              Geometry::CloudOfPoints const& t_points );

    Geometry::CloudOfPoints
    solve_RK4_movable_vortices( double dt, CartesianGridOfSpeed& t_velocity, 
                                Simulation::Vortices& t_vortices, 
                                Geometry::CloudOfPoints const& t_points );
}

#endif