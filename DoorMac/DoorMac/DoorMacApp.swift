import SwiftUI
import AppKit
import FirebaseCore

@main
struct DoorMacApp: App {
    init() {
        FirebaseApp.configure()
    }

    @StateObject private var serialMonitor = SerialMonitor()
    
    var body: some Scene {
        WindowGroup {
            SensorGaugesView(serialMonitor: serialMonitor)
        }
    }
}
