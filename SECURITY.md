# Security, safety and misuse reporting

**POWERED BY [mikey:hexoid-sys]**  
Project owner: **Yogesh R. Chauhan (mikey-7x)** — https://github.com/mikey-7x

This is experimental embedded and RF hardware rather than a network service. The most important reports concern electrical safety, radio-frequency compliance, misleading measurement claims and unauthorised use of the project identity.

## Report a safety or correctness problem

Open a GitHub issue in the official repository and include:

- board and MCU marking;
- firmware settings and splash-screen clock/range lines;
- input waveform, voltage and source details;
- displayed state, gate label and raw `N`;
- photographs or measurements that reproduce the problem;
- whether the input was disconnected, moving, clipped or ringing.

Do not post private personal information. Do not post instructions intended to evade radio regulations.

## RF and electrical safety

- The higher STM32 clocks used by parts of this project exceed the MCU manufacturer's 72 MHz system-clock specification.
- Never exceed PA0 electrical limits or connect an unknown oscillator without checking its DC and peak voltage.
- The FM-transmitter circuit can radiate RF energy. Use a rated dummy load or shielded bench arrangement and operate only where the operator, equipment, frequency, power and emissions are authorised.
- Treat `MHz EXP` as an experimental counted-edge result until independently confirmed.

## Report licence or identity misuse

If you see this project being sold, distributed without the required mikey-7x credit/logo, presented as another person's original work, or used commercially without permission, contact Yogesh R. Chauhan (mikey-7x) through:

- GitHub profile: https://github.com/mikey-7x
- Project repository: https://github.com/mikey-7x/298.666mhz-frequency-counter-using-stm32f103c8t6-and-vhf-uhf-fm-transmitter-circuit

Include the URL, seller/account name, screenshots and dates. Commercial permission is never granted by silence or by access to this public repository.
