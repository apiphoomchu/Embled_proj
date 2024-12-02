import SwiftUI
// ContentView.swift
struct ContentView: View {
    @StateObject private var serialMonitor = SerialMonitor()
    
    var body: some View {
        SensorGaugesView(serialMonitor: serialMonitor)
            // Force view to update when these values change
            .id(serialMonitor.lightIntensity)
            .id(serialMonitor.distance)
    }
}

struct SensorGaugesView: View {
    @ObservedObject var serialMonitor: SerialMonitor
    
    var body: some View {
        VStack(spacing: 30) {
            Text("Sensor Readings")
                .font(.title)
                .fontWeight(.bold)
            
            HStack(spacing: 40) {
                // Light Intensity Gauge
                VStack {
                    Gauge(value: Double(serialMonitor.lightIntensity), in: 0...1023) {
                        Label("Light", systemImage: "sun.max.fill")
                    } currentValueLabel: {
                        Text("\(serialMonitor.lightIntensity)")
                            .font(.system(.body, design: .monospaced))
                    }
                    .gaugeStyle(.accessoryCircular)
                    .tint(Gradient(colors: [.yellow, .orange]))
                    .scaleEffect(2.0)
                    .frame(width: 150, height: 150)
                    
                    Text("Light Intensity")
                        .font(.headline)
                        .padding(.top, 20)
                    
                    Text("\(serialMonitor.lightIntensity) / 1023")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
                .onChange(of: serialMonitor.lightIntensity) { newValue in
                    print("Light intensity updated to: \(newValue)")
                }
                
                // Distance Gauge
                VStack {
                    Gauge(value: Double(serialMonitor.distance), in: 0...400) {
                        Label("Distance", systemImage: "ruler.fill")
                    } currentValueLabel: {
                        Text("\(serialMonitor.distance)")
                            .font(.system(.body, design: .monospaced))
                    }
                    .gaugeStyle(.accessoryCircular)
                    .tint(Gradient(colors: [.blue, .cyan]))
                    .scaleEffect(2.0)
                    .frame(width: 150, height: 150)
                    
                    Text("Distance")
                        .font(.headline)
                        .padding(.top, 20)
                    
                    Text("\(serialMonitor.distance) cm")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
                .onChange(of: serialMonitor.distance) { newValue in
                    print("Distance updated to: \(newValue)")
                }
            }
            .padding()
            
            // Debug Values
            VStack(alignment: .leading, spacing: 10) {
                Text("Debug Info:")
                    .font(.headline)
                Text("Light: \(serialMonitor.lightIntensity)")
                Text("Distance: \(serialMonitor.distance)")
                Text("Status: \(serialMonitor.isConnected ? "Connected" : "Disconnected")")
            }
            .padding()
            .background(Color.gray.opacity(0.1))
            .cornerRadius(10)
            
            // Connection Status
            HStack(spacing: 10) {
                Circle()
                    .fill(serialMonitor.isConnected ? Color.green : Color.red)
                    .frame(width: 10, height: 10)
                
                Text(serialMonitor.isConnected ? "Connected" : "Disconnected")
                    .foregroundColor(serialMonitor.isConnected ? .green : .red)
            }
            .padding()
            
            // Connect/Disconnect Button
            Button(action: {
                if serialMonitor.isConnected {
                    serialMonitor.disconnect()
                } else {
                    serialMonitor.connect()
                }
            }) {
                Text(serialMonitor.isConnected ? "Disconnect" : "Connect")
                    .foregroundColor(.white)
                    .padding(.horizontal, 30)
                    .padding(.vertical, 10)
                    .background(serialMonitor.isConnected ? Color.red : Color.blue)
                    .cornerRadius(20)
            }
        }
        .padding()
    }
}

// Preview Provider
struct SensorGaugesView_Previews: PreviewProvider {
    static var previews: some View {
        SensorGaugesView(serialMonitor: SerialMonitor())
    }
}
