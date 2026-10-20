# TIM1 三相互补 PWM

- TIM1 时钟 160 MHz，PSC=0，中心对齐模式1，ARR=5000：16 kHz。
- CKD=DIV1，DTG=96：600 ns 死区。600 ns 是配置目标，需实测确认。
- PA8/PA9/PA10：CH1/CH2/CH3，AF6。
- PB13/PB14：CH1N/CH2N，AF6；PB15：CH3N，AF4。
- PWM1，主/互补输出均高电平有效，三相初始 CCR=2500。
- CH4 保持 PWM2、CCR4=4800、OC4REF→TRGO→ADC1 注入组。

ADC_TestStart 内部已调用 TIM1_ThreePhase_Init，不需再次初始化。
启动顺序仍然是 MX_ADC1_Init、MX_TIM1_Init、ADC_TestStart。
ADC_TestStart 会启动采样，但六路功率输出仍保持 GPIO 低电平。

需要输出时，在 main.c 的 ADC_TestStart 成功之后调用：

```c
if (TIM1_ThreePhase_SetCompare(2500U, 2500U, 2500U) != HAL_OK)
{
    Error_Handler();
}
if (TIM1_ThreePhase_Start() != HAL_OK)
{
    Error_Handler();
}
```

停止三相输出，保留 ADC 采样：

```c
TIM1_ThreePhase_Stop();
```

修改占空比（原始比较值，范围0..5000）：

```c
TIM1_ThreePhase_SetCompare(2000U, 2500U, 3000U);
```

新值在自然更新事件时通过CCR预装载生效，接口不修改CCR4或触发UG。
中心对齐且RCR=0时，自然更新事件可能每半周期一次；将来电流控制算法需明确更新时刻。
本模块直接整体使能/关闭CCER中的六路相输出；不要再单独对CH1-3调用HAL启动停止函数。
运行中不要重复调用ADC_TestStart，也不要调用HAL_TIM_PWM_Init重新初始化共享定时器。

Watch：tim1_pwm_running=0为关闭，1为输出已使能。
count_test 和 adc_raw 保留原有采样调试方式。
若停止后重新启动，保留上次比较值；需要50%时先设置2500、2500、2500。
当前没有新增硬件Break输入：软件Stop不能替代硬件过流保护。
固定CCR4=4800并不保证任意占空比下的低侧电流采样窗口有效。

验证：新增模块随工程经Keil ARMCC编译链接；输出频率、极性、死区及停止电平需上板测量。

比较值仅由 ADC 中断调用 SetCompare 更新。PWM 模块已移除全局中断屏蔽，保留 UDIS 防止三路比较值分批装载。启动和停止请串行调用，避免在不同中断层级同时操作。

模块已合并到 adc_injected.c 和 adc_injected.h，main.c 只需包含 adc_injected.h。原 TIM1_ThreePhase_* 接口名称及行为保持不变。
