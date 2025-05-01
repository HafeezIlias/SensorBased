// Import the functions you need from the SDKs
import { initializeApp } from "https://www.gstatic.com/firebasejs/9.6.1/firebase-app.js";
import { getDatabase, ref, set, onValue } from "https://www.gstatic.com/firebasejs/9.6.1/firebase-database.js";

// Firebase configuration
const firebaseConfig = {
    apiKey: "AIzaSyDlcY8ulZWXQUfHhJHd0vbuUD0vM-6k4bA",
    authDomain: "sensorbased-16dee.firebaseapp.com",
    databaseURL: "https://sensorbased-16dee-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "sensorbased-16dee",
    storageBucket: "sensorbased-16dee.firebasestorage.app",
    messagingSenderId: "259658818719",
    appId: "1:259658818719:web:a644edf37bd931ceb3eeaa",
    measurementId: "G-ZBGT95EQDN"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

// Reference to the database value
const valueRef = ref(database, "value");

window.updateDatabase = function() {
  // Generate a random number to simulate a value update
  const newValue = Math.floor(Math.random() * 100) + 1;
  // Update the value in the database
  set(valueRef, newValue);
}

// Listen for real-time changes
onValue(valueRef, (snapshot) => {
  const value = snapshot.val();
  document.getElementById("value-display").innerText = value;
});
  