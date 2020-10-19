#pragma once

#define limit(min, x, max) {x < min ? min : (x > max ? max :x)}

class MotorControl {
public:
    MotorControl() :
        integrator(0.0),
        output(0.0),
        targetRpm(0.0),
        lastError(0.0)
    {

    }

    void set_speed(float rpm) {
        targetRpm = rpm;
    }

    float update(float current_rpm) {
        if(targetRpm < 1.0) {
            output = OffPwm;
        } else {
            float e = limit(-ErrorLimit, targetRpm - current_rpm, ErrorLimit);
            
            integrator += e * Ki;
            integrator = limit(-IMax, integrator, IMax);
            output = MinPwm + Kp * e + integrator + Kd * (e - lastError);
            output = limit(MinPwm, output, MaxPwm);
            lastError = e;
        }
        return output;
    }

private:
    float integrator;
    float output;
    float targetRpm;
    float lastError;

    static constexpr float OffPwm = 1000.0;
    static constexpr float MinPwm = 1050.0;
    static constexpr float MaxPwm = 1800.0;
    static constexpr float IMax = 150.0;
    static constexpr float Kp = 1.0;
    static constexpr float Ki = 0.005;
    static constexpr float Kd = 0.3;
    static constexpr float ErrorLimit = 75;
 };