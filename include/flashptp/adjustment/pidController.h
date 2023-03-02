/*
 * @file pidController.h
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
 *
 * This class implements a PID controller adjustment algorithm (@see Adjustment.h).
 * Unlike many other PID controller implementations, this one applies an integral
 * adjustment part by keeping a small part of the previous adjustment when
 * performing a new adjustment with a proportional and (optional) differential
 * part. Ratios of all parts (iRatio, pRatio, dRatio) as well as a step threshold
 * in nanoseconds can be configured, individually.
 *
 * Minimum:     i = 0.001, p = 0.01, d = 0
 * Maximum:     i = 0.5, p = 1, d = 1
 * Default:     i = 0.05, p = 0.2, d = 0
 *
 * =============================================================================
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * =============================================================================
 *
 */

#ifndef INCLUDE_FLASHPTP_ADJUSTMENT_PIDCONTROLLER_H_
#define INCLUDE_FLASHPTP_ADJUSTMENT_PIDCONTROLLER_H_

#include <flashptp/adjustment/adjustment.h>

#define FLASH_PTP_JSON_CFG_PID_CONTROLLER_PROPORTIONAL_RATIO        "proportionalRatio"
#define FLASH_PTP_JSON_CFG_PID_CONTROLLER_P_RATIO                   "pRatio"
#define FLASH_PTP_JSON_CFG_PID_CONTROLLER_INTEGRAL_RATIO            "integralRatio"
#define FLASH_PTP_JSON_CFG_PID_CONTROLLER_I_RATIO                   "iRatio"
#define FLASH_PTP_JSON_CFG_PID_CONTROLLER_DIFFERENTIAL_RATIO        "differentialRatio"
#define FLASH_PTP_JSON_CFG_PID_CONTROLLER_D_RATIO                   "dRatio"
#define FLASH_PTP_JSON_CFG_PID_CONTROLLER_STEP_THRESHOLD            "stepThreshold"

#define FLASH_PTP_PID_CONTROLLER_P_RATIO_MIN                        0.01
#define FLASH_PTP_PID_CONTROLLER_P_RATIO_DEFAULT                    0.2
#define FLASH_PTP_PID_CONTROLLER_P_RATIO_MAX                        1

#define FLASH_PTP_PID_CONTROLLER_I_RATIO_MIN                        0.005
#define FLASH_PTP_PID_CONTROLLER_I_RATIO_DEFAULT                    0.05
#define FLASH_PTP_PID_CONTROLLER_I_RATIO_MAX                        0.5

#define FLASH_PTP_PID_CONTROLLER_D_RATIO_MIN                        0
#define FLASH_PTP_PID_CONTROLLER_D_RATIO_DEFAULT                    0
#define FLASH_PTP_PID_CONTROLLER_D_RATIO_MAX                        1

#define FLASH_PTP_PID_CONTROLLER_STEP_THRESHOLD_DEFAULT             1000000

namespace flashptp {
namespace adjustment {

class PIDController : public Adjustment {
public:
    inline PIDController(const std::string &clockName = "", clockid_t clockID = -1) :
        Adjustment(AdjustmentType::pidController, clockName, clockID) { }

    static bool validateConfig(const Json &config, std::vector<std::string> *errs);
    virtual void setConfig(const Json &config);

    bool adjust(std::vector<client::Server*> &servers);

    /*
     * Clear the calculations of all servers after an adjustment has been applied,
     * but only if ki (integral ratio) is zero or if the step threshold has been exceeded.
     */
    void finalize(const std::vector<client::Server*> &servers);

private:
    // Ratio (gain factor) of the proportional control output value (applied to the measured offset).
    double _kp{ FLASH_PTP_PID_CONTROLLER_P_RATIO_DEFAULT };
    /*
     * Ratio of the integral control output value. In this PID controller implementation,
     * the integral value is applied by reverting only a part of the previous adjustment.
     * This ratio defines the part of the previous adjustment that is to be kept.
     *
     * That means, that the size of the integral control output depends on all of the
     * configurable ratios (kp, ki or kd) of the PID controller.
     */
    double _ki{ FLASH_PTP_PID_CONTROLLER_I_RATIO_DEFAULT };
    // Ratio of the differential control output value (applied to the measured drift).
    double _kd{ FLASH_PTP_PID_CONTROLLER_D_RATIO_DEFAULT };

    double _integral{ 0 };
    double _proportional{ 0 };
    double _differential{ 0 };

    // Offset threshold (ns) indicating that - if exceeded - a clock step is to be applied
    uint64_t _stepThreshold{ FLASH_PTP_PID_CONTROLLER_STEP_THRESHOLD_DEFAULT };
};

}
}

#endif /* INCLUDE_FLASHPTP_ADJUSTMENT_PROPOFFSET_H_ */
