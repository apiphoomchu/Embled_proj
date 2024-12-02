import Foundation
import ORSSerial
import SwiftUI
import AVFoundation
import FirebaseFirestore
import UserNotifications

// MARK: - Serial Monitor Class
final class SerialMonitor: NSObject, ObservableObject {
    // MARK: - Published Properties
    @Published var distance: Int = 0
    @Published var lightIntensity: Int = 0
    @Published var isConnected: Bool = false
    @Published var detectedPerson: Bool = false
    @Published var lastLogTime: String = ""
    
    // MARK: - Private Properties
    private var dataBuffer = ""
    private var periodTimer: Timer?
    private let maxBufferSize = 100
    private let serialPortManager = ORSSerialPortManager.shared()
    private let googleScriptURL = "YOUR_KEY"
    private var audioPlayer: AVAudioPlayer?
    private var lastLogTimestamp: Date?
    private let minLogInterval: TimeInterval = 5
    private let db = Firestore.firestore()
    
    private var serialPort: ORSSerialPort? {
        didSet {
            oldValue?.close()
            oldValue?.delegate = nil
            serialPort?.delegate = self
        }
    }
    
    // MARK: - Initialization
    override init() {
        super.init()
        setupAudio()
        requestNotificationPermission()
    }
    
    // MARK: - Audio Setup
    private func setupAudio() {
        guard let soundURL = Bundle.main.url(forResource: "alert", withExtension: "mp3") else { return }
        do {
            audioPlayer = try AVAudioPlayer(contentsOf: soundURL)
            audioPlayer?.prepareToPlay()
        } catch {
            print("Audio setup failed: \(error)")
        }
    }
    
    // MARK: - Notification Setup
    private func requestNotificationPermission() {
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .sound, .badge]) { granted, error in
            if granted {
                print("Notification permission granted")
            } else if let error = error {
                print("Notification permission error: \(error)")
            }
        }
    }
    
    // MARK: - Connection Management
    func connect() {
        if let port = serialPortManager.availablePorts.first(where: { $0.path.contains("usbserial") }) {
            serialPort = port
            serialPort?.baudRate = 115200
            serialPort?.dtr = true
            serialPort?.rts = true
            serialPort?.open()
            
            periodTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
                self?.appendPeriodAndProcess()
            }
        }
    }
    
    func disconnect() {
        periodTimer?.invalidate()
        periodTimer = nil
        serialPort?.close()
    }
    
    // MARK: - Alert Sound and Notification
    private func playAlert() {
        audioPlayer?.play()
        
        // Create and send notification
        let content = UNMutableNotificationContent()
        content.title = "Person Detected!"
        content.body = "Someone is within proximity (Distance: \(distance)cm, Light: \(lightIntensity))"
        content.sound = .default
        
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )
        
        UNUserNotificationCenter.current().add(request) { error in
            if let error = error {
                print("Notification error: \(error)")
            }
        }
    }
    
    // MARK: - Data Logging
    private func logData(distance: Int, lightIntensity: Int) {
        if let lastLog = lastLogTimestamp,
           Date().timeIntervalSince(lastLog) < minLogInterval {
            return
        }
        
        let timestamp = Date()
        let isoFormatter = ISO8601DateFormatter()
        let isoTimestamp = isoFormatter.string(from: timestamp)
        
        // Format timestamp for display
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "HH:mm:ss"
        let displayTime = dateFormatter.string(from: timestamp)
        
        // Log to Firebase
        let docData: [String: Any] = [
            "distance": distance,
            "lightIntensity": lightIntensity,
            "timestamp": Timestamp(date: timestamp),
        ]
        
        db.collection("proximityLogs").addDocument(data: docData) { [weak self] err in
            if let err = err {
                print("Firebase logging error: \(err)")
            } else {
                DispatchQueue.main.async {
                    self?.lastLogTime = "Last logged at: \(displayTime)"
                }
            }
        }
        
        // Log to Google Sheets
        let parameters: [String: Any] = [
            "distance": distance,
            "lightIntensity": lightIntensity,
            "timestamp": isoTimestamp,
        ]
        
        guard let url = URL(string: googleScriptURL) else { return }
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        
        do {
            request.httpBody = try JSONSerialization.data(withJSONObject: parameters)
            
            URLSession.shared.dataTask(with: request) { [weak self] data, response, error in
                if let error = error {
                    print("Google Sheets logging error: \(error)")
                    return
                }
                
                if let httpResponse = response as? HTTPURLResponse,
                   httpResponse.statusCode == 200 {
                    DispatchQueue.main.async {
                        self?.lastLogTimestamp = timestamp
                    }
                }
            }.resume()
        } catch {
            print("Request preparation error: \(error)")
        }
    }
    
    // MARK: - Buffer Processing
    private func appendPeriodAndProcess() {
        dataBuffer += "."
        processBuffer()
    }
    
    private func processBuffer() {
        let pattern = "li\\d+di\\d+"
        if let range = dataBuffer.range(of: pattern, options: .regularExpression) {
            let match = String(dataBuffer[range])
            
            if let lightMatch = match.range(of: "li(\\d+)", options: .regularExpression),
               let distanceMatch = match.range(of: "di(\\d+)", options: .regularExpression) {
                
                let lightStr = match[lightMatch].dropFirst(2)
                let distanceStr = match[distanceMatch].dropFirst(2)
                
                if let light = Int(lightStr),
                   let distance = Int(distanceStr) {
                    DispatchQueue.main.async { [weak self] in
                        guard let self = self else { return }
                        self.lightIntensity = light
                        self.distance = distance
                        
                        let personDetected = distance < 35
                        if personDetected != self.detectedPerson {
                            self.detectedPerson = personDetected
                            if personDetected {
                                self.playAlert()
                                self.logData(distance: distance, lightIntensity: light)
                            }
                        }
                    }
                }
            }
            
            dataBuffer = String(dataBuffer[range.upperBound...])
        }
        
        if dataBuffer.count > maxBufferSize {
            dataBuffer = String(dataBuffer.suffix(maxBufferSize / 2))
        }
    }
}

// MARK: - Serial Port Delegate
extension SerialMonitor: ORSSerialPortDelegate {
    func serialPort(_ serialPort: ORSSerialPort, didReceive data: Data) {
        guard let string = String(data: data, encoding: .utf8) else { return }
        dataBuffer += string
        processBuffer()
    }
    
    func serialPortWasOpened(_ serialPort: ORSSerialPort) {
        DispatchQueue.main.async { [weak self] in
            self?.isConnected = true
        }
    }
    
    func serialPortWasClosed(_ serialPort: ORSSerialPort) {
        periodTimer?.invalidate()
        periodTimer = nil
        DispatchQueue.main.async { [weak self] in
            self?.isConnected = false
        }
    }
    
    func serialPortWasRemovedFromSystem(_ serialPort: ORSSerialPort) {
        periodTimer?.invalidate()
        periodTimer = nil
        DispatchQueue.main.async { [weak self] in
            guard let self = self else { return }
            self.isConnected = false
            self.serialPort = nil
            self.detectedPerson = false
        }
    }
}
