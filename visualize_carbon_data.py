#!/usr/bin/env python3
"""
Single-tier Carbon Trading Network Visualization
Generates graphs from the single-tier simulation results
"""

import matplotlib.pyplot as plt
import numpy as np

# Sample data for 10 sensors (single-tier)
sensor_ids = [f'S{i+1}' for i in range(10)]
avg_co2 = [405, 475, 470, 568, 593, 629, 680, 734, 825, 838]

fig, ax = plt.subplots(figsize=(12, 6))
bars = ax.bar(sensor_ids, avg_co2, color='skyblue', edgecolor='black')
ax.set_xlabel('Sensor ID', fontweight='bold')
ax.set_ylabel('Average CO2 (ppm)', fontweight='bold')
ax.set_title('Single-Tier CO2 Sensor Network - Average CO2 per Sensor', 
             fontweight='bold')
ax.grid(axis='y', alpha=0.3)
ax.axhline(y=400, color='green', linestyle='--', linewidth=2, 
            label='Normal (400 ppm)', alpha=0.6)

for bar, val in zip(bars, avg_co2):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 10,
             f'{val}', ha='center', va='bottom', fontweight='bold')

plt.tight_layout()
output_file = 'carbon_trading_visualization.png'
plt.savefig(output_file, dpi=300, bbox_inches='tight')
print(f"✓ Single-tier visualization saved as: {output_file}")

plt.show()
