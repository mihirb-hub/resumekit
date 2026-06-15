# Heron Power Application Response

**Prompt:** Please describe three technical accomplishments that demonstrate you are an exceptional candidate for this role. Include adequate detail explaining the problem you faced and how you solved it in a unique or impressive way.

---

### 1. Owning the Design and Validation Loop for Custom EV Power Electronics
**The Problem:** While working on the Formula Slug electric vehicle team, we needed a custom, low-voltage PCB to handle SAE J1772 charge signaling. The board had to flawlessly manage power regulation and fault detection to ensure vehicle safety, but we were constrained by strict spatial requirements and a tight ~$80 BOM. 
**The Solution:** I owned the entire development loop from schematic capture to physical validation. I architected the board using KiCad 9, carefully sourcing and categorizing components to meet our budget. Because safety and measurement integrity were critical, I designed a high-voltage test bench setup featuring programmable power supplies and oscilloscopes. I implemented safety interlocks and defined grounding strategies to safely execute rigorous functional testing and reliability campaigns on our TSMP-based power regulation circuits. 
**Why it makes me a fit:** This demonstrates my ability to design complex power electronics and, more importantly, my hands-on capability to build the safe, reliable test bench infrastructure required to validate them under real-world stress conditions.

### 2. Automating Data Workflows and Instrument Control for Hardware Validation
**The Problem:** At the Santa Cruz Institute for Particle Physics (SCIPP), our team was validating complex custom-PCB LED boards and optical sensors. We faced a severe bottleneck: executing high-precision validation measurements manually was slow, prone to human error, and made isolating hardware anomalies incredibly tedious.
**The Solution:** I took the initiative to replace our manual processes by building a completely automated, Python-based test infrastructure. I integrated Raspberry Pi DAQ systems to control our instruments and handle real-time data logging. To close the loop and make the data actionable, I architected workflows using Python and MATLAB that automatically generated plots and structured test summaries. When we encountered signal anomalies, I used these automated logs to drive fast, first-principles debug loops and isolate the root cause.
**Why it makes me a fit:** Heron Power needs someone who can build data workflows to make test results actionable. My experience proving out automated DAQ systems and generating trustworthy, automated plots directly mirrors your goal of turning raw bench data into verified fixes.

### 3. Rapid Prototyping and Integration in a Scrappy, High-Stakes Environment
**The Problem:** As a Vision Engineer for Slugbotics, we had a strict deadline to integrate complex electro-mechanical sensor hardware (cameras and data pipelines) into an underwater vehicle (MATE ROV). The hardware had to survive a highly unforgiving, dynamic physical environment, and our initial setups were too fragile for field deployment.
**The Solution:** Operating in a fast-paced environment where we had to learn by doing, I led the rapid prototyping of waterproof mechanical assemblies. I simultaneously wrote the Python scripts required to process the real-time visual data. When we hit integration hurdles between the software and hardware, I drove aggressive debug loops, instrumenting deeper and iterating on the physical design until it was bulletproof.
**Why it makes me a fit:** I thrive in environments with a strong bias toward hands-on execution. I know how to take a scrappy, first-iteration setup, troubleshoot the physical and software failure cliffs, and turn it into a robust, production-quality system that actually works in the field.
