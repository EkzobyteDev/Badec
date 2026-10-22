"""Л4.№2: длина сжатых методом Хаффмана данных при разной разрядности частот.

Для каждого файла считаются ненормализованные частоты байтов, затем для каждой
разрядности B = 1..64 частоты приводятся к диапазону 0..2^B-1, по ним строятся
коды Хаффмана и рассчитываются:
    E_B — длина сжатых данных в байтах;
    G_B = E_B + 256*B/8 = E_B + 32*B — длина сжатых данных вместе с таблицей частот.
B* — разрядность, при которой G_B минимальна.
Результаты и коды Хаффмана каждого байта (при B = 64) записываются в results/result.txt,
графики E_B/E_64 и G_B/G_64 сохраняются в results/.

Запуск:
    python huffman_bits.py файл1 [файл2 ...]
    python lab4_task2/huffman_bits.py a.txt anecdotes.txt LICENSE makefile huffman/badec.c 
    huffman/header.h v2/undec_v2.cpp badec huffman_tests otik-labs.pdf
"""

import argparse
import contextlib
import heapq
import math
import sys
from collections import Counter
from pathlib import Path

ALPHABET_LENGTH = 256
ALL_WIDTHS = tuple(range(1, 65))  #

    #Ненормализованные частоты: count(a) для каждого байта a = 0..255.
def count_bytes(path):
    counter = Counter()
    with open(path, "rb") as file:
        while chunk := file.read(1 << 16): #Читаем по 64 КБ 
            counter.update(chunk)
    return [counter[byte] for byte in range(ALPHABET_LENGTH)]


def normalize(counts, width):
    #Приводит частоты к диапазону 0..2^width-1.

    limit = (1 << width) - 1
    max_count = max(counts)
    if max_count <= limit:
        return list(counts)
    return [0 if c == 0 else max(1, (c * limit + max_count // 2) // max_count)
            for c in counts]


def huffman_code_lengths(frequencies):
    """Длины кодов Хаффмана для каждого байта (0 — у байта нет кода).

    Уточнения при построении дерева:
      - на каждом шаге объединяются два узла с наименьшей частотой;
      - при равных частотах раньше берётся лист с меньшим значением байта,
        а листья берутся раньше внутренних узлов; внутренние узлы — в порядке
        их создания;
      - если в файле встречается только один байт, его код имеет длину 1 бит.
    """
    lengths = [0] * ALPHABET_LENGTH
    # Узел кучи: (частота, порядок при равенстве частот, байты в поддереве).
    heap = [(f, byte, [byte]) for byte, f in enumerate(frequencies) if f > 0]
    if len(heap) == 1:
        lengths[heap[0][2][0]] = 1
        return lengths
    heapq.heapify(heap) #Сортирует кучу

    next_order = ALPHABET_LENGTH
    while len(heap) > 1:
        freq_a, _, bytes_a = heapq.heappop(heap) #Удаляет и возвращает минимальный элемент
        freq_b, _, bytes_b = heapq.heappop(heap)
        for byte in bytes_a + bytes_b:
            lengths[byte] += 1
        heapq.heappush(heap, (freq_a + freq_b, next_order, bytes_a + bytes_b))
        next_order += 1
    return lengths


def canonical_codes(lengths):
    """Канонические коды Хаффмана по длинам кодов (None — у байта нет кода).

    Байты сортируются по длине кода, при равной длине — по значению байта.
    Первый получает код из нулей, каждый следующий — предыдущий код + 1;
    если длина выросла, код дополняется нулями справа (сдвиг влево).
    Длины те же, что у дерева, поэтому и длина сжатых данных та же.
    """
    codes = [None] * ALPHABET_LENGTH
    code = 0
    prev_length = 0
    for length, byte in sorted((l, b) for b, l in enumerate(lengths) if l > 0):
        code <<= length - prev_length
        codes[byte] = format(code, f"0{length}b")
        code += 1
        prev_length = length
    return codes


def information_bits(counts):
    """Оценка Л2.№1: I = sum count(a) * log2(n / count(a)) бит."""
    n = sum(counts)
    return sum(c * math.log2(n / c) for c in counts if c > 0)


def analyze_file(path):
    counts = count_bytes(path)
    n = sum(counts)
    rows = {}
    for width in ALL_WIDTHS:
        lengths = huffman_code_lengths(normalize(counts, width))
        bits = sum(c * l for c, l in zip(counts, lengths))
        e = math.ceil(bits / 8)
        g = e + ALPHABET_LENGTH * width // 8
        rows[width] = {"bits": bits, "E": e, "G": g}
    return {
        "path": Path(path),
        "counts": counts,
        "n": n,
        "distinct": sum(1 for c in counts if c > 0),
        "I": information_bits(counts),
        "rows": rows,
    }


def best_width(result, widths):
    """B* = argmin G_B; при равенстве — меньшая разрядность."""
    return min(widths, key=lambda w: (result["rows"][w]["G"], w))


def ratio(value, base):
    return value / base if base else float("nan")


def print_result(result):
    rows = result["rows"]
    e64, g64 = rows[64]["E"], rows[64]["G"]
    info = result["I"]
    lower_e = math.ceil(info / 8)

    print(f"Файл: {result['path']}")
    print(f"  n = {result['n']} байт, различных байтов: {result['distinct']}")
    print(f"  Оценка Л2.№1: I = {info:.2f} бит = {info / 8:.2f} октетов;"
          f" E >= {lower_e}, G64 >= {lower_e + 256 * 8}, G8 >= {lower_e + 256}")
    print(f"  {'B':>3} | {'E_B, байт':>12} | {'G_B, байт':>12} | {'E_B/E_64':>9} | {'G_B/G_64':>9}")
    for width in ALL_WIDTHS:
        row = rows[width]
        print(f"  {width:>3} | {row['E']:>12} | {row['G']:>12} |"
              f" {ratio(row['E'], e64):>9.4f} | {ratio(row['G'], g64):>9.4f}")

    best = best_width(result, ALL_WIDTHS)
    print(f"  B* = {best} (G = {rows[best]['G']} байт)")
    print()


def print_codes(result, width):
    """Таблица кодов Хаффмана для каждого встречающегося байта при разрядности width."""
    counts = result["counts"]
    frequencies = normalize(counts, width)
    lengths = huffman_code_lengths(frequencies)
    codes = canonical_codes(lengths)

    print(f"  Коды Хаффмана при B = {width}:")
    print(f"  {'байт':>4} | {'симв':>4} | {'count':>9} | {'ν_B':>9} | {'длина':>5} | код")
    for byte in range(ALPHABET_LENGTH):
        if counts[byte] == 0:
            continue
        symbol = chr(byte) if 32 <= byte < 127 else ""
        print(f"  0x{byte:02X} | {symbol:>4} | {counts[byte]:>9} | {frequencies[byte]:>9} |"
              f" {lengths[byte]:>5} | {codes[byte]}")
    print()


def plot(results, out_dir):
    """+2 балла: один график E_B/E_64 и один G_B/G_64 от B для всех файлов."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.ticker import ScalarFormatter

    results = [r for r in results if r["rows"][64]["E"] > 0]  # пустые файлы не на что делить
    out_dir.mkdir(parents=True, exist_ok=True)
    widths = sorted(ALL_WIDTHS)
    colors = plt.get_cmap("tab20").colors
    markers = "o^sDvP*X<>"

    for key, title, log_scale in (("E", "E_B / E_64", False), ("G", "G_B / G_64", True)):
        fig, ax = plt.subplots(figsize=(13, 6.5))
        all_values = []
        for i, result in enumerate(results):
            rows = result["rows"]
            values = [rows[w][key] / rows[64][key] for w in widths]
            all_values += values
            ax.plot(widths, values, color=colors[i % len(colors)],
                    marker=markers[i % len(markers)], markersize=3, linewidth=1.2,
                    label=f"{result['path'].name} ({result['n']} Б)")
        ax.set_xlabel("B — разрядность частоты, бит")
        ax.set_ylabel(title)
        ax.set_title(f"Хаффман: {title} в зависимости от разрядности частот B")
        ax.set_xticks([1, 2, 4, 8, 12, 16, 24, 32, 40, 48, 56, 64])
        if log_scale:
            # G_B/G_64 у маленьких файлов падает в десятки раз — логарифмическая ось
            ax.set_yscale("log")
            low, high = min(all_values) / 1.1, max(all_values) * 1.1
            ax.set_yticks([t for t in (0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.5, 0.7, 1, 1.5, 2, 3, 5)
                           if low <= t <= high])
            ax.set_ylim(low, high)
            ax.yaxis.set_major_formatter(ScalarFormatter())
        ax.grid(True, which="both", alpha=0.3)
        ax.legend(fontsize=8, loc="upper left", bbox_to_anchor=(1.01, 1))
        fig.tight_layout()
        path = out_dir / f"{key}_ratio.png"
        fig.savefig(path, dpi=150)
        plt.close(fig)
        print(f"График сохранён: {path}")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

    parser = argparse.ArgumentParser(description="Л4.№2: коды Хаффмана при разной разрядности частот")
    parser.add_argument("files", nargs="+", type=Path, help="анализируемые файлы")
    args = parser.parse_args()

    results_dir = Path(__file__).resolve().parent / "results"
    results_dir.mkdir(parents=True, exist_ok=True)

    results = []
    with open(results_dir / "result.txt", "w", encoding="utf-8") as result_file:
        with contextlib.redirect_stdout(result_file):
            for path in args.files:
                if not path.is_file():
                    print(f"Пропущен {path}: не файл", file=sys.stderr)
                    continue
                result = analyze_file(path)
                if result["n"] == 0:
                    print(f"Пропущен {path}: файл пустой", file=sys.stderr)
                    continue
                results.append(result)
                print_result(result)
                print_codes(result, 64)

    if results:
        plot(results, results_dir)
        print(f"Результаты сохранены: {results_dir / 'result.txt'}")


if __name__ == "__main__":
    main()
