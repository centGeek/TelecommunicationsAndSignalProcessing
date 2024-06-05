import heapq
import socket
from collections import Counter
from bitarray import bitarray


class Node:
    def __init__(self, char, freq):
        self.char = char
        self.freq = freq
        self.left = None
        self.right = None

    def __lt__(self, other):
        return self.freq < other.freq


def build_huffman_tree(text):
    freq = Counter(text)
    heap = [Node(char, f) for char, f in freq.items()]
    heapq.heapify(heap)
    while len(heap) > 1:
        left = heapq.heappop(heap)
        right = heapq.heappop(heap)
        merged = Node(None, left.freq + right.freq)
        merged.left = left
        merged.right = right
        heapq.heappush(heap, merged)
    return heap[0]


def build_huffman_codes(node, prefix="", codebook=None):
    if codebook is None:
        codebook = {}
    if node.char is not None:
        codebook[node.char] = prefix
    else:
        build_huffman_codes(node.left, prefix + "0", codebook)
        build_huffman_codes(node.right, prefix + "1", codebook)
    return codebook


def huffman_encode(text, codebook):
    encoded = bitarray()
    for char in text:
        encoded.extend(bitarray(codebook[char]))
    return encoded


def read_message_from_file(filepath):
    with open(filepath, 'r', newline='') as file:
        return file.read()


def save_message_to_file(message, filepath):
    with open(filepath, 'w', newline='') as file:
        file.write(message)


def save_encoded_message_to_file(encoded_message, filepath):
    with open(filepath, 'wb') as file:
        encoded_message.tofile(file)


def save_codebook_to_file(codebook, filepath):
    with open(filepath, 'w', newline='') as file:
        for char, code in codebook.items():
            file.write(f"{repr(char)}:{code}\n")


def send_file(filename, host, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        with open(filename, 'rb') as file:
            s.sendfile(file)


def receive_file(filename, host, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, port))
        s.listen(1)
        conn, addr = s.accept()
        with conn:
            with open(filename, 'wb') as file:
                while True:
                    data = conn.recv(1024)
                    if not data:
                        break
                    file.write(data)


def load_codebook_from_file(filepath):
    codebook = {}
    with open(filepath, 'r', newline='') as file:
        for line in file:
            char, code = line.strip().split(':')
            codebook[eval(char)] = code
    return codebook


def invert_codebook(codebook):
    return {code: char for char, code in codebook.items()}


def huffman_decode(encoded_message, codebook):
    inverted_codebook = invert_codebook(codebook)
    decoded_message = []
    buffer = ''
    for bit in encoded_message:
        buffer += '1' if bit else '0'
        if buffer in inverted_codebook:
            decoded_message.append(inverted_codebook[buffer])
            buffer = ''
    decoded_text = ''.join(decoded_message)
    if decoded_text.endswith('n'):
        decoded_text = decoded_text[:-1]
    return decoded_text

def main_sender(host, port1, port2):
    original_filepath = "OryginalnaWiadomość.txt"
    encoded_filepath = "ZakodowanaWiadomość.bin"
    codebook_filepath = "huffman_codebook.txt"

    text = read_message_from_file(original_filepath)

    huffman_tree = build_huffman_tree(text)
    codebook = build_huffman_codes(huffman_tree)
    encoded_text = huffman_encode(text, codebook)

    save_encoded_message_to_file(encoded_text, encoded_filepath)
    save_codebook_to_file(codebook, codebook_filepath)

    send_file(encoded_filepath, host, port1)
    send_file(codebook_filepath, host, port2)


def main_receiver(host, port1, port2):
    encoded_filepath = "OdebranaZakodowanaWiadomość.bin"
    codebook_filepath = "OdebranyCodebook.txt"

    receive_file(encoded_filepath, host, port1)
    receive_file(codebook_filepath, host, port2)

    encoded_message = bitarray()
    with open(encoded_filepath, 'rb') as file:
        encoded_message.fromfile(file)

    codebook = load_codebook_from_file(codebook_filepath)
    decoded_message = huffman_decode(encoded_message, codebook)

    decoded_filepath = "odkodowanaWiadomość.txt"
    save_message_to_file(decoded_message, decoded_filepath)


def show_menu():
    print("Łukasz Centkowski 247638")
    print("Maciej Dominiak 247644")
    print("1. Send message")
    print("2. Receive message")
    print("3. Exit")
    choice = input("Choose an option: ")
    return choice


def main():
    default_port1 = 65432
    default_port2 = 65433

    while True:
        choice = show_menu()
        if choice == '1':
            host = input("Enter the IP address of the receiver: ")
            port1 = input(f"Enter the port for sending encoded message (default {default_port1}): ")
            port2 = input(f"Enter the port for sending the codebook (default {default_port2}): ")

            port1 = int(port1) if port1 else default_port1
            port2 = int(port2) if port2 else default_port2

            main_sender(host, port1, port2)
        elif choice == '2':
            host = input("Enter your IP address: ")
            port1 = input(f"Enter the port for receiving encoded message (default {default_port1}): ")
            port2 = input(f"Enter the port for receiving the codebook (default {default_port2}): ")

            port1 = int(port1) if port1 else default_port1
            port2 = int(port2) if port2 else default_port2

            main_receiver(host, port1, port2)
        elif choice == '3':
            break
        else:
            print("Invalid choice, please try again.")


if __name__ == "__main__":
    main()
