import math
import statistics
from collections import Counter, defaultdict
from pathlib import Path

from openpyxl import Workbook, load_workbook
from openpyxl.utils import get_column_letter


def percentile(sorted_values, fraction):
    if not sorted_values:
        return None

    if len(sorted_values) == 1:
        return sorted_values[0]

    position = (len(sorted_values) - 1) * fraction
    lower = math.floor(position)
    upper = math.ceil(position)

    if lower == upper:
        return sorted_values[lower]

    weight = position - lower

    return (
        sorted_values[lower] * (1 - weight)
        + sorted_values[upper] * weight
    )


def get_iqr_limits(values):
    sorted_values = sorted(values)

    if len(sorted_values) < 4:
        return None, None

    q1 = percentile(sorted_values, 0.25)
    q3 = percentile(sorted_values, 0.75)
    iqr = q3 - q1

    return q1 - 1.5 * iqr, q3 + 1.5 * iqr


def approximate_mode(values):
    if not values:
        return None, None

    counts = Counter(values)
    value, count = counts.most_common(1)[0]

    if count > 1:
        return value, "exata"

    if len(values) < 3 or min(values) == max(values):
        return value, "exata"

    sorted_values = sorted(values)
    q1 = percentile(sorted_values, 0.25)
    q3 = percentile(sorted_values, 0.75)
    iqr = q3 - q1

    if iqr > 0:
        bin_width = 2 * iqr / (len(values) ** (1 / 3))
        bin_count = math.ceil(
            (max(values) - min(values)) / bin_width
        ) if bin_width > 0 else 0
    else:
        bin_count = math.ceil(math.log2(len(values)) + 1)

    bin_count = max(1, min(bin_count, 100))

    minimum = min(values)
    maximum = max(values)
    width = (maximum - minimum) / bin_count

    if width == 0:
        return minimum, "exata"

    bins = [0] * bin_count

    for item in values:
        index = int((item - minimum) / width)
        index = min(index, bin_count - 1)
        bins[index] += 1

    max_count = max(bins)
    best_bin = bins.index(max_count)
    mode_value = minimum + (best_bin + 0.5) * width

    return mode_value, "aproximada (histograma)"


def calculate_stats(values):
    values = [
        float(value)
        for value in values
        if isinstance(value, (int, float))
        and not isinstance(value, bool)
        and math.isfinite(value)
    ]

    if not values:
        return None

    values_sorted = sorted(values)
    outlier_low, outlier_high = get_iqr_limits(values_sorted)

    if outlier_low is None:
        clean_values = values[:]
        outliers = []
    else:
        outliers = [
            value
            for value in values
            if value < outlier_low or value > outlier_high
        ]

        clean_values = [
            value
            for value in values
            if outlier_low <= value <= outlier_high
        ]

    def describe(sample):
        if not sample:
            return {
                "count": 0,
                "sum": 0.0,
                "mean": None,
                "median": None,
                "mode": None,
                "mode_type": None,
                "stddev": None,
                "min": None,
                "max": None,
            }

        mode, mode_type = approximate_mode(sample)

        return {
            "count": len(sample),
            "sum": sum(sample),
            "mean": statistics.fmean(sample),
            "median": statistics.median(sample),
            "mode": mode,
            "mode_type": mode_type,
            "stddev": statistics.stdev(sample) if len(sample) > 1 else 0.0,
            "min": min(sample),
            "max": max(sample),
        }

    return {
        "count": len(values),
        "outlier_count": len(outliers),
        "outlier_percent": (
            len(outliers) / len(values) * 100
            if values else 0
        ),
        "iqr_lower": outlier_low,
        "iqr_upper": outlier_high,
        "all": describe(values),
        "clean": describe(clean_values),
    }


def write_sheet(workbook, name, headers, rows):
    worksheet = workbook.create_sheet(name)
    worksheet.append(headers)

    for row in rows:
        worksheet.append(list(row))

    worksheet.freeze_panes = "A2"

    if headers and worksheet.max_row > 1:
        last_column = get_column_letter(len(headers))
        last_row = worksheet.max_row

        worksheet.auto_filter.ref = (
            f"A1:{last_column}{last_row}"
        )

    for column in worksheet.columns:
        max_length = 0
        column_letter = get_column_letter(column[0].column)

        for cell in column:
            if cell.value is not None:
                max_length = max(
                    max_length,
                    len(str(cell.value))
                )

        worksheet.column_dimensions[column_letter].width = min(
            max_length + 2,
            40
        )


def read_rows(workbook, sheet_name):
    if sheet_name not in workbook.sheetnames:
        print(f"[AVISO] Planilha '{sheet_name}' não encontrada.")
        return []

    worksheet = workbook[sheet_name]
    rows = worksheet.iter_rows(values_only=True)

    try:
        headers = list(next(rows))
    except StopIteration:
        return []

    data = []

    for row in rows:
        data.append(dict(zip(headers, row)))

    return data


def summarize_groups(groups, metric_type, default_unit):
    summary_rows = []

    for metric_name in sorted(
        groups,
        key=lambda item: str(item)
    ):
        values = groups[metric_name]

        # Ajustes refinados de unidade e escala baseados no nome da métrica
        unit = default_unit
        if "[Throughput %]" in metric_name:
            unit = "%"
        elif "Clock Frequency" in metric_name:
            unit = "Hz"
        elif "Requests" in metric_name or "Count" in metric_name:
            unit = "requests"

        stats = calculate_stats(values)

        if stats is None:
            continue

        all_stats = stats["all"]
        clean_stats = stats["clean"]

        summary_rows.append([
            metric_type,
            metric_name,
            unit,
            stats["count"],
            all_stats["sum"],
            all_stats["mean"],
            all_stats["median"],
            all_stats["mode"],
            all_stats["mode_type"],
            all_stats["stddev"],
            all_stats["min"],
            all_stats["max"],
            clean_stats["count"],
            clean_stats["sum"] if "clean" in locals() else None,
            clean_stats["mean"],
            clean_stats["median"],
            clean_stats["mode"],
            clean_stats["mode_type"],
            clean_stats["stddev"],
            clean_stats["min"],
            clean_stats["max"],
            stats["outlier_count"],
            stats["outlier_percent"],
            stats["iqr_lower"],
            stats["iqr_upper"],
        ])

    return summary_rows


def export_gpu_metric_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "GPU_Metrics")
    if not rows:
        return

    groups = defaultdict(list)
    for row in rows:
        metric_name = row.get("metric_name")
        if metric_name is None:
            metric_name = (
                f"type_id={row.get('type_id')}; "
                f"metric_id={row.get('metric_id')}"
            )
        value = row.get("value")
        if not isinstance(value, (int, float)) or not math.isfinite(value):
            continue
        groups[str(metric_name)].append(float(value))

    result_summary = summarize_groups(
        groups,
        "GPU_Metric",
        "unidade Nsight"
    )
    summary_rows.extend(result_summary)
    print(f"Métricas GPU analisadas: {len(groups)}")


def export_nvml_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "NVML_Power_Temp")
    if not rows:
        return

    groups_power = defaultdict(list)
    groups_temp = defaultdict(list)
    groups_other = defaultdict(list)

    for row in rows:
        metric_name = row.get("metric_name") or "Unknown"
        value = row.get("value")
        try:
            fval = float(value)
        except (TypeError, ValueError):
            continue
        if not math.isfinite(fval):
            continue

        if "Power" in metric_name:
            groups_power[str(metric_name)].append(fval)
        elif "Temperature" in metric_name:
            groups_temp[str(metric_name)].append(fval)
        else:
            groups_other[str(metric_name)].append(fval)

    if groups_power:
        summary_rows.extend(summarize_groups(groups_power, "NVML_Power", "W"))
    if groups_temp:
        summary_rows.extend(summarize_groups(groups_temp, "NVML_Temperature", "°C"))
    if groups_other:
        summary_rows.extend(summarize_groups(groups_other, "NVML_Metric", "unidade NVML"))


def export_nvtx_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "NVTX")
    if not rows:
        return

    groups = defaultdict(list)
    for row in rows:
        name = row.get("text") or "(sem nome)"
        value = row.get("duration_ms")
        if not isinstance(value, (int, float)) or not math.isfinite(value):
            continue
        groups[str(name)].append(float(value))

    summary_rows.extend(summarize_groups(groups, "NVTX_Duration", "ms"))


def export_vulkan_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "Vulkan_API")
    if not rows:
        return

    groups = defaultdict(list)
    for row in rows:
        name = row.get("function") or "(sem nome)"
        value = row.get("duration_ms")
        if not isinstance(value, (int, float)) or not math.isfinite(value):
            continue
        groups[str(name)].append(float(value))

    summary_rows.extend(summarize_groups(groups, "Vulkan_API_Duration", "ms"))


def export_frame_time_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "Vulkan_API")
    if not rows:
        return

    present_rows = [
        r for r in rows
        if r.get("function") == "vkQueuePresentKHR" and isinstance(r.get("start"), (int, float))
    ]
    present_rows.sort(key=lambda x: x["start"])

    gpu_frame_durations = [
        (present_rows[i]["start"] - present_rows[i-1]["start"]) / 1_000_000.0
        for i in range(1, len(present_rows))
        if 0 < (present_rows[i]["start"] - present_rows[i-1]["start"]) / 1_000_000.0 < 500
    ]

    acquire_rows = [
        r for r in rows
        if r.get("function") == "vkAcquireNextImageKHR" and isinstance(r.get("start"), (int, float))
    ]
    acquire_rows.sort(key=lambda x: x["start"])

    cpu_frame_durations = [
        (acquire_rows[i]["start"] - acquire_rows[i-1]["start"]) / 1_000_000.0
        for i in range(1, len(acquire_rows))
        if 0 < (acquire_rows[i]["start"] - acquire_rows[i-1]["start"]) / 1_000_000.0 < 500
    ]

    if gpu_frame_durations:
        summary_rows.extend(summarize_groups(
            {"GPU Frame Duration (Present Delta)": gpu_frame_durations},
            "Frame_Time",
            "ms"
        ))

    if cpu_frame_durations:
        summary_rows.extend(summarize_groups(
            {"CPU Frame Duration (Acquire Delta)": cpu_frame_durations},
            "Frame_Time",
            "ms"
        ))


def export_context_switch_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "Context_Switches")
    if not rows:
        return

    timestamps = [r.get("timestamp") for r in rows if isinstance(r.get("timestamp"), (int, float))]

    if timestamps:
        summary_rows.append([
            "Context_Switches",
            "GPU Context Switch Count",
            "count",
            len(timestamps),
            float(len(timestamps)),
            None, None, None, None, None,
            min(timestamps), max(timestamps),
            len(timestamps),
            float(len(timestamps)),
            None, None, None, None, None, None, None,
            0, 0.0, None, None
        ])


def export_vulkan_synchronization_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "Vulkan_API")
    if not rows:
        return

    sync_functions = {'vkWaitForFences', 'vkDeviceWaitIdle', 'vkCreateFence', 'vkCreateSemaphore'}
    sync_values = []

    for row in rows:
        func_name = row.get("function")
        if func_name in sync_functions:
            val = row.get("duration_ms")
            if isinstance(val, (int, float)) and math.isfinite(val):
                sync_values.append(float(val))

    if sync_values:
        summary_rows.extend(summarize_groups(
            {"Vulkan Synchronization APIs (Wait/Fences/Semaphores)": sync_values},
            "Vulkan_Synchronization",
            "ms"
        ))


def export_vulkan_memory_analysis(raw_workbook, summary_rows):
    rows = read_rows(raw_workbook, "Vulkan_Memory")
    if not rows:
        return

    duration_values = []
    size_values = []

    for row in rows:
        dur = row.get("duration_ms")
        if isinstance(dur, (int, float)) and math.isfinite(dur):
            duration_values.append(float(dur))

        size = row.get("memory_size_bytes")
        if isinstance(size, (int, float)) and math.isfinite(size):
            size_values.append(float(size))

    if duration_values:
        summary_rows.extend(summarize_groups(
            {"Vulkan Memory Operation Duration": duration_values},
            "GPU_Memory",
            "ms"
        ))

    if size_values:
        summary_rows.extend(summarize_groups(
            {"Vulkan Memory Allocation Size": size_values},
            "GPU_Memory",
            "bytes"
        ))


def main():
    raw_files = sorted(Path(".").glob("*_dados.xlsx"))
    if not raw_files:
        print("Nenhum arquivo '*_dados.xlsx' encontrado na pasta atual.")
        return

    raw_file = raw_files[0]
    output_file = raw_file.with_name(
        raw_file.stem.replace("_dados", "") + "_analise.xlsx"
    )

    print(f"Arquivo de entrada: {raw_file}")
    print(f"Arquivo de saída: {output_file}")

    raw_workbook = load_workbook(
        raw_file,
        read_only=True,
        data_only=True
    )

    summary_rows = []

    try:
        export_gpu_metric_analysis(raw_workbook, summary_rows)
        export_nvml_analysis(raw_workbook, summary_rows)
        export_nvtx_analysis(raw_workbook, summary_rows)
        export_vulkan_analysis(raw_workbook, summary_rows)
        export_frame_time_analysis(raw_workbook, summary_rows)
        export_context_switch_analysis(raw_workbook, summary_rows)
        export_vulkan_synchronization_analysis(raw_workbook, summary_rows)
        export_vulkan_memory_analysis(raw_workbook, summary_rows)
    finally:
        raw_workbook.close()

    if not summary_rows:
        print("[ERRO] Nenhuma amostra numérica adequada para análise.")
        return

    workbook = Workbook()
    workbook.remove(workbook.active)

    summary_headers = [
        "tipo_metrica",
        "nome_metrica",
        "unidade",
        "quantidade_amostras",
        "soma",
        "media",
        "mediana",
        "moda",
        "tipo_moda",
        "desvio_padrao",
        "minimo",
        "maximo",
        "quantidade_amostras_sem_outliers",
        "soma_sem_outliers",
        "media_sem_outliers",
        "mediana_sem_outliers",
        "moda_sem_outliers",
        "tipo_moda_sem_outliers",
        "desvio_padrao_sem_outliers",
        "minimo_sem_outliers",
        "maximo_sem_outliers",
        "quantidade_outliers",
        "percentual_outliers",
        "limite_inferior_iqr",
        "limite_superior_iqr",
    ]

    write_sheet(
        workbook,
        "Metric_Summary",
        summary_headers,
        summary_rows
    )

    workbook.save(output_file)

    print(f"\nAnálise concluída com sucesso: {output_file}")
    print(f"Métricas analisadas: {len(summary_rows)}")


if __name__ == "__main__":
    main()
