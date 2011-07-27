function cur_state = quad_model_step(dT, cur_state)
% Advance one step in time the quadcopter model
%
% JC 2011-07-22

q = cur_state.q;
actuator = cur_state.Actuator;

thrust = (actuator(1:4) - 1000) * 0.01;

t1 = thrust(4) - thrust(2);
t2 = thrust(1) - thrust(3);
t3 = thrust(1) + thrust(3) - thrust(2) - thrust(4);

torque = [t1; t2; t3];

thrust_body = [0; 0; mean(thrust)];
thrust_Rbe = [-2 * (q(2) * q(4) - q(1) * q(3));
              -2 * (q(3) * q(4) + q(1) * q(2));
              -(q(1) * q(1) - q(2)*q(2) - q(3)*q(3) + q(4)*q(4))];
thrust_ned = cross(thrust_body, thrust_Rbe);
thrust_ned(3) = thrust_ned(3) - 9.81;

qdot = [-q(2) -q(3) -q(4);
    q(1) -q(4) q(3);
    q(4) q(1) -q(2);
    -q(3) q(2) q(1)] * torque;

q = q + qdot * dT;
q = q / norm(q);

cur_state.q = q;
cur_state.Gyros = torque * 2 * 180 / pi;  % TODO: Need real conversion from torque to deg/s
cur_state.Accels = thrust_ned;
cur_state.Velocity = cur_state.Velocity + cur_state.Accels * dT;
cur_state.Position = cur_state.Position + cur_state.Velocity * dT;

