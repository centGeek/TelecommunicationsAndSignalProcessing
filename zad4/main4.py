import tkinter as tk
from tkinter import ttk, messagebox
import soundcard as sc
import numpy as np
import soundfile as sf
from scipy.signal import resample

def record_audio(filename, duration):
    sample_rate = 48000
    bit_depth = 32

    # Get the default microphone
    mic = sc.default_microphone()

    # Record audio
    print("Recording...")
    audio_data = mic.record(samplerate=sample_rate, numframes=sample_rate * duration)
    print("Finished recording.")

    # Save the recorded data as a WAV file
    sf.write(filename, audio_data, samplerate=sample_rate, subtype='PCM_32')

    return audio_data

def play_audio(filename, sample_rate, bit_depth):
    # Load the recorded data from the WAV file
    audio_data, file_sample_rate = sf.read(filename, dtype='float32')

    # Resample the audio if the sample rate is different
    if file_sample_rate != sample_rate:
        num_samples = int(len(audio_data) * sample_rate / file_sample_rate)
        audio_data = resample(audio_data, num_samples)

    # Normalize the audio data to the range of -1.0 to 1.0
    max_val = np.max(np.abs(audio_data))
    if max_val > 0:
        audio_data = audio_data / max_val

    # Scale the audio data to the desired bit depth
    if bit_depth == 8:
        dtype = np.int8
        max_int = 2**7 - 1
        audio_data = (audio_data * max_int).astype(dtype)
    elif bit_depth == 16:
        dtype = np.int16
        max_int = 2**15 - 1
        audio_data = (audio_data * max_int).astype(dtype)
    elif bit_depth == 24:
        dtype = np.int32
        max_int = 2**23 - 1
        audio_data = (audio_data * max_int).astype(dtype)
    elif bit_depth == 32:
        dtype = np.int32
        max_int = 2**31 - 1
        audio_data = (audio_data * max_int).astype(dtype)
    else:
        raise ValueError("Unsupported bit depth")

    # Normalize audio data back to range [-1, 1]
    audio_data = audio_data.astype('float32') / max_int

    # Get the default speaker
    spk = sc.default_speaker()

    # Play the audio data
    print("Playing audio...")
    spk.play(audio_data, samplerate=sample_rate)
    print("Finished playing.")

def calculate_theoretical_snr(bit_depth):
    # Calculate theoretical quantization noise SNR
    snr_quant = 6.02 * bit_depth + 1.76
    return snr_quant

def start_recording():
    filename = filename_entry.get()
    duration = int(duration_entry.get())

    try:
        audio_data = record_audio(filename, duration)
        snr_quant = calculate_theoretical_snr(32)  # since we always record in 32-bit
        messagebox.showinfo("Recording Complete", f"Recording saved to {filename}\nTheoretical Quantization SNR: {snr_quant:.2f} dB")
    except ValueError as e:
        messagebox.showerror("Error", str(e))

def start_playing():
    filename = filename_entry.get()
    sample_rate = int(play_sample_rate_scale.get())
    bit_depth = int(play_bit_depth_var.get())

    try:
        play_audio(filename, sample_rate, bit_depth)
    except ValueError as e:
        messagebox.showerror("Error", str(e))

# Create the main window
root = tk.Tk()
root.title("Audio Recorder and Player")

# Create and place the widgets for recording
ttk.Label(root, text="Filename:").grid(row=0, column=0, padx=10, pady=5)
filename_entry = ttk.Entry(root, width=30)
filename_entry.grid(row=0, column=1, padx=10, pady=5)
filename_entry.insert(0, "output.wav")

ttk.Label(root, text="Duration (seconds):").grid(row=1, column=0, padx=10, pady=5)
duration_entry = ttk.Entry(root, width=10)
duration_entry.grid(row=1, column=1, padx=10, pady=5)
duration_entry.insert(0, "5")

record_button = ttk.Button(root, text="Record", command=start_recording)
record_button.grid(row=2, column=0, columnspan=2, pady=10)

# Create and place the widgets for playback
ttk.Label(root, text="Playback Sample Rate (Hz):").grid(row=3, column=0, padx=10, pady=5)
play_sample_rate_scale = tk.Scale(root, from_=8000, to=48000, orient=tk.HORIZONTAL)
play_sample_rate_scale.set(44100)
play_sample_rate_scale.grid(row=3, column=1, padx=10, pady=5)

ttk.Label(root, text="Playback Bit Depth:").grid(row=4, column=0, padx=10, pady=5)
play_bit_depth_var = tk.IntVar(value=16)
play_bit_depth_combobox = ttk.Combobox(root, textvariable=play_bit_depth_var, state="readonly")
play_bit_depth_combobox['values'] = (8, 16, 24, 32)
play_bit_depth_combobox.grid(row=4, column=1, padx=10, pady=5)

play_button = ttk.Button(root, text="Play", command=start_playing)
play_button.grid(row=5, column=0, columnspan=2, pady=10)

# Start the GUI event loop
root.mainloop()
