/*
 * MIT License
 *
 * Copyright (c) 2024 Kouhei Ito
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef PID_HPP
#define PID_HPP

class PID {
   private:
    float m_kp;
    float m_ti;
    float m_td;
    float m_eta;
    float m_err, m_err2, m_err3;
    float m_h;

   public:
    float m_differential;
    float m_integral;
    PID();
    void set_parameter(float kp, float ti, float td, float eta, float h);
    void reset(void);
    void i_reset(void);
    void printGain(void);
    void set_error(float err);
    float update(float err, float h);
};

class Filter {
   private:
    float m_state;
    float m_T;
    float m_h;

   public:
    float m_out;
    Filter();
    void set_parameter(float T, float h);
    void reset(void);
    float update(float u, float h);
};


class WashoutFilter {
private:
    float b0, b1; // 分子係数
    float a0, a1; // 分母係数
    float x_prev; // 入力の1サンプル前
    float y_prev; // 出力の1サンプル前

public:
    // コンストラクタ
    WashoutFilter(float timeConstant, float samplingTime) {
        // 双一次変換で係数を計算
        float T = timeConstant;
        float Ts = samplingTime;
        b0 = (2 * T) / (2 * T + Ts);
        b1 = -b0;
        a0 = 1.0;
        a1 = -(2 * T - Ts) / (2 * T + Ts);

        // 初期化
        x_prev = 0.0;
        y_prev = 0.0;
    }

    // フィルタリング処理
    float update(double input) {
        // 双一次変換の差分方程式
        float output = (b0 * input + b1 * x_prev - a1 * y_prev) / a0;

        // 過去値を更新
        x_prev = input;
        y_prev = output;

        return output;
    }
};




#endif