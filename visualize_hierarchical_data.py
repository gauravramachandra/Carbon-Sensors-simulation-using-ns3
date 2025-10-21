#!/usr/bin/env python3
"""
Hierarchical Carbon Trading Network Visualization
Generates graphs from the hierarchical simulation results
"""

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle, FancyBboxPatch, Circle
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D

# Hierarchical network data (10 sensors, 5 zones, 5 APs, 1 main gateway)
zones = ['Zone 1', 'Zone 2', 'Zone 3', 'Zone 4', 'Zone 5']
sensors_per_zone = 2
total_sensors = 10

# Sample CO2 data by zone (derived from simulation output)
zone_data = {
    'Zone 1': {'sensors': ['S1', 'S2'], 'avg_co2': [405, 475], 'color': '#FF6B6B'},
    'Zone 2': {'sensors': ['S3', 'S4'], 'avg_co2': [470, 568], 'color': '#4ECDC4'},
    'Zone 3': {'sensors': ['S5', 'S6'], 'avg_co2': [593, 629], 'color': '#45B7D1'},
    'Zone 4': {'sensors': ['S7', 'S8'], 'avg_co2': [680, 734], 'color': '#95E1D3'},
    'Zone 5': {'sensors': ['S9', 'S10'], 'avg_co2': [825, 838], 'color': '#F38181'}
}

# Network performance metrics
total_packets_sent = 60
total_packets_received = 60
delivery_ratio = 100.0
num_zones = 5
num_aps = 5

# Create figure with multiple subplots
fig = plt.figure(figsize=(18, 12))
fig.suptitle('Hierarchical Carbon Trading Network - Simulation Results', 
             fontsize=18, fontweight='bold')

# 1. CO2 Levels by Zone (grouped bar chart)
ax1 = plt.subplot(2, 3, 1)
x = np.arange(len(zones))
width = 0.35

sensor1_data = [zone_data[z]['avg_co2'][0] for z in zones]
sensor2_data = [zone_data[z]['avg_co2'][1] for z in zones]

bars1 = ax1.bar(x - width/2, sensor1_data, width, label='Sensor 1', 
                alpha=0.8, edgecolor='black')
bars2 = ax1.bar(x + width/2, sensor2_data, width, label='Sensor 2', 
                alpha=0.8, edgecolor='black')

# Color bars by zone
for i, (bar1, bar2) in enumerate(zip(bars1, bars2)):
    bar1.set_color(zone_data[zones[i]]['color'])
    bar2.set_color(zone_data[zones[i]]['color'])
    bar2.set_alpha(0.6)

ax1.set_xlabel('Zone', fontweight='bold', fontsize=11)
ax1.set_ylabel('Average CO2 (ppm)', fontweight='bold', fontsize=11)
ax1.set_title('CO2 Levels by Zone and Sensor', fontweight='bold', fontsize=12)
ax1.set_xticks(x)
ax1.set_xticklabels(zones)
ax1.legend()
ax1.grid(axis='y', alpha=0.3)
ax1.axhline(y=400, color='green', linestyle='--', linewidth=2, 
            label='Normal (400 ppm)', alpha=0.5)

# 2. Average CO2 per Zone
ax2 = plt.subplot(2, 3, 2)
zone_averages = [(zone_data[z]['avg_co2'][0] + zone_data[z]['avg_co2'][1]) / 2 
                 for z in zones]
zone_colors = [zone_data[z]['color'] for z in zones]

bars2 = ax2.bar(zones, zone_averages, color=zone_colors, alpha=0.7, 
                edgecolor='black', linewidth=2)
ax2.set_xlabel('Zone', fontweight='bold', fontsize=11)
ax2.set_ylabel('Average CO2 (ppm)', fontweight='bold', fontsize=11)
ax2.set_title('Zone-Level Average CO2 Emissions', fontweight='bold', fontsize=12)
ax2.grid(axis='y', alpha=0.3)

for bar, val in zip(bars2, zone_averages):
    ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 15,
             f'{val:.1f}', ha='center', va='bottom', fontweight='bold', fontsize=10)

# 3. Network Performance Metrics
ax3 = plt.subplot(2, 3, 3)
metrics = ['Packets\nSent', 'Packets\nReceived', 'Delivery\nRatio (%)', 
           'Active\nZones', 'Local\nAPs']
values = [total_packets_sent, total_packets_received, delivery_ratio, num_zones, num_aps]
colors_perf = ['#FFB347', '#77DD77', '#84C1FF', '#DDA0DD', '#F0E68C']
bars3 = ax3.bar(metrics, values, color=colors_perf, alpha=0.7, edgecolor='black', linewidth=2)
ax3.set_ylabel('Count / Percentage', fontweight='bold', fontsize=11)
ax3.set_title('Hierarchical Network Performance', fontweight='bold', fontsize=12)
ax3.grid(axis='y', alpha=0.3)

for bar, val in zip(bars3, values):
    ax3.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1.5,
             f'{val:.0f}', ha='center', va='bottom', fontweight='bold', fontsize=10)

# 4. CO2 Distribution Across All Sensors
ax4 = plt.subplot(2, 3, 4)
all_sensors = []
all_co2 = []
all_colors = []

for zone in zones:
    for i, sensor in enumerate(zone_data[zone]['sensors']):
        all_sensors.append(f"{sensor}\n({zone})")
        all_co2.append(zone_data[zone]['avg_co2'][i])
        all_colors.append(zone_data[zone]['color'])

bars4 = ax4.bar(range(len(all_sensors)), all_co2, color=all_colors, 
                alpha=0.7, edgecolor='black', linewidth=1)
ax4.set_xlabel('Sensor (Zone)', fontweight='bold', fontsize=11)
ax4.set_ylabel('Average CO2 (ppm)', fontweight='bold', fontsize=11)
ax4.set_title('Individual Sensor CO2 Readings', fontweight='bold', fontsize=12)
ax4.set_xticks(range(len(all_sensors)))
ax4.set_xticklabels(all_sensors, rotation=45, ha='right', fontsize=8)
ax4.grid(axis='y', alpha=0.3)
ax4.axhline(y=400, color='green', linestyle='--', linewidth=1.5, alpha=0.5)

# 5. Packets per Zone
ax5 = plt.subplot(2, 3, 5)
packets_per_zone = [12, 12, 12, 12, 12]  # 2 sensors × 6 readings each
bars5 = ax5.bar(zones, packets_per_zone, color=zone_colors, alpha=0.7, 
                edgecolor='black', linewidth=2)
ax5.set_xlabel('Zone', fontweight='bold', fontsize=11)
ax5.set_ylabel('Packets Transmitted', fontweight='bold', fontsize=11)
ax5.set_title('Data Collection by Zone', fontweight='bold', fontsize=12)
ax5.grid(axis='y', alpha=0.3)

for bar, val in zip(bars5, packets_per_zone):
    ax5.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.3,
             f'{val}', ha='center', va='bottom', fontweight='bold', fontsize=10)

# 6. Hierarchical Network Topology Diagram
ax6 = plt.subplot(2, 3, 6)
ax6.set_xlim(0, 120)
ax6.set_ylim(0, 80)
ax6.set_aspect('equal')
ax6.axis('off')
ax6.set_title('Hierarchical Network Architecture', fontweight='bold', fontsize=12)

# Draw main gateway at top center
main_gw_x, main_gw_y = 60, 70
main_gw = FancyBboxPatch((main_gw_x-8, main_gw_y-4), 16, 8, 
                         boxstyle="round,pad=0.1", 
                         facecolor='darkgreen', edgecolor='black', 
                         linewidth=3, alpha=0.8)
ax6.add_patch(main_gw)
ax6.text(main_gw_x, main_gw_y, 'Main\nGateway', ha='center', va='center', 
         fontweight='bold', fontsize=9, color='white')

# Draw CSMA backbone
ax6.plot([10, 110], [55, 55], 'b-', linewidth=4, alpha=0.5, label='CSMA Backbone')
ax6.text(60, 58, 'CSMA Backbone (10.2.1.0/24)', ha='center', 
         fontsize=8, style='italic', bbox=dict(boxstyle='round', 
         facecolor='lightblue', alpha=0.5))

# Draw zones with local APs and sensors
zone_positions = [15, 33, 51, 69, 87]  # X positions for 5 zones

for i, (zone_x, zone_name) in enumerate(zip(zone_positions, zones)):
    zone_color = zone_data[zone_name]['color']
    
    # Draw local AP
    ap_y = 45
    ap_circle = Circle((zone_x, ap_y), 3, color='orange', 
                      alpha=0.8, edgecolor='black', linewidth=2)
    ax6.add_patch(ap_circle)
    ax6.text(zone_x, ap_y, f'AP{i+1}', ha='center', va='center', 
             fontweight='bold', fontsize=7, color='white')
    
    # Connect AP to backbone
    ax6.plot([zone_x, zone_x], [ap_y+3, 55-2], 'b-', linewidth=2, alpha=0.7)
    
    # Draw two sensors below AP
    sensor_positions = [(zone_x - 4, 25), (zone_x + 4, 25)]
    
    for j, (sx, sy) in enumerate(sensor_positions):
        sensor_circle = Circle((sx, sy), 2.5, color=zone_color, 
                              alpha=0.7, edgecolor='black', linewidth=1.5)
        ax6.add_patch(sensor_circle)
        
        sensor_idx = i * 2 + j + 1
        ax6.text(sx, sy, f'S{sensor_idx}', ha='center', va='center', 
                 fontweight='bold', fontsize=7, color='white')
        
        # Connect sensor to AP with WiFi indicator
        ax6.plot([sx, zone_x], [sy+2.5, ap_y-3], 'g--', 
                linewidth=1.5, alpha=0.5)
        
        # Add CO2 reading
        co2_val = zone_data[zone_name]['avg_co2'][j]
        ax6.text(sx, sy - 5, f'{co2_val:.0f}', ha='center', 
                fontsize=6, style='italic', 
                bbox=dict(boxstyle='round,pad=0.2', 
                         facecolor='white', alpha=0.7))
    
    # Zone label
    ax6.text(zone_x, 15, zone_name, ha='center', fontweight='bold', 
             fontsize=8, bbox=dict(boxstyle='round', 
             facecolor=zone_color, alpha=0.3))
    
    # WiFi network label
    wifi_net = f'10.1.{i+1}.0/24'
    ax6.text(zone_x, 35, wifi_net, ha='center', fontsize=6, 
             style='italic', color='green')

# Connect main gateway to backbone
ax6.plot([main_gw_x, main_gw_x], [main_gw_y-4, 55+2], 'b-', 
         linewidth=3, alpha=0.7)

# Add legend
legend_elements = [
    mpatches.Patch(color='darkgreen', label='Main Gateway'),
    mpatches.Patch(color='orange', label='Local APs (5)'),
    mpatches.Patch(color='gray', label='Sensors (10)'),
        Line2D([0], [0], color='blue', linewidth=3, label='CSMA (Ethernet)'),
        Line2D([0], [0], color='green', linewidth=2, 
              linestyle='--', label='WiFi 802.11b')
]
ax6.legend(handles=legend_elements, loc='lower right', fontsize=7)

# Add simulation info
info_text = (
    'Network Configuration:\n'
    '• 10 CO2 Sensors (2 per zone)\n'
    '• 5 Local WiFi APs\n'
    '• 1 Main Gateway\n'
    '• 5 WiFi Networks\n'
    '• 1 CSMA Backbone\n'
    '• Static Routing\n'
    '• 30s simulation\n'
    '• 100% Delivery'
)
props = dict(boxstyle='round', facecolor='wheat', alpha=0.4)
ax6.text(2, 78, info_text, fontsize=6.5, verticalalignment='top', bbox=props)

plt.tight_layout()
output_file = 'hierarchical_carbon_visualization.png'
plt.savefig(output_file, dpi=300, bbox_inches='tight')
print(f"✓ Hierarchical network visualization saved as: {output_file}")
print(f"✓ Total sensors: {total_sensors}")
print(f"✓ Zones: {num_zones}")
print(f"✓ Local APs: {num_aps}")
print(f"✓ Packets sent: {total_packets_sent}")
print(f"✓ Packets received: {total_packets_received}")
print(f"✓ Delivery ratio: {delivery_ratio}%")

plt.show()
