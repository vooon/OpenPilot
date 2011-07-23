function cur_state = quad_model_step(dT, cur_state)
% Advance one step in time the quadcopter model
%
% JC 2011-07-22

q = cur_state.q;

q = q + actuator(1:4) * dT;
q = q / norm(q);

cur_state.q = q;