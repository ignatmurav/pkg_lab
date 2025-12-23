import cv2
import numpy as np
import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image, ImageTk
import os

class ImageProcessor:
    def __init__(self, root):
        self.root = root
        self.root.title("ЛР 3 - Вариант 6")
        self.root.geometry("1400x800")
        
        self.image = None
        self.processed_image = None
        self.gray_image = None
        
        # Параметры
        self.processing_type = tk.StringVar(value="elementwise")
        
        self.create_widgets()

    def create_widgets(self):
        # Верхняя панель управления (компактная)
        top_frame = tk.Frame(self.root, height=40)
        top_frame.pack(fill=tk.X, padx=5, pady=2)
        
        # Кнопки
        tk.Button(top_frame, text="Загрузить", command=self.load_image, width=10).pack(side=tk.LEFT, padx=2)
        tk.Button(top_frame, text="Сохранить", command=self.save_image, width=10).pack(side=tk.LEFT, padx=2)
        tk.Button(top_frame, text="Применить", command=self.process_image, width=10, bg="#e0e0e0").pack(side=tk.LEFT, padx=2)
        
        # Выбор типа обработки
        types_frame = tk.Frame(top_frame)
        types_frame.pack(side=tk.LEFT, padx=20)
        
        types = [
            ("Поэлемент", "elementwise"),
            ("Контраст", "contrast"),
            ("Глоб.порог", "global_threshold"),
            ("Адапт.порог", "adaptive_threshold")
        ]
        
        for text, value in types:
            tk.Radiobutton(types_frame, text=text, variable=self.processing_type, 
                          value=value, command=self.update_parameter_panel).pack(side=tk.LEFT, padx=3)
        
        # Параметры
        self.param_frame = tk.Frame(top_frame)
        self.param_frame.pack(side=tk.LEFT, padx=10)
        self.create_parameter_panel()
        
        # Основная область - БОЛЬШИЕ окна для изображений
        main_frame = tk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Оригинал (левая половина)
        left_frame = tk.Frame(main_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=2)
        
        tk.Label(left_frame, text="Оригинальное изображение", font=("Arial", 11)).pack()
        self.original_label = tk.Label(left_frame, bg="white", relief=tk.SUNKEN, bd=1)
        self.original_label.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)
        
        # Результат (правая половина)
        right_frame = tk.Frame(main_frame)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=2)
        
        tk.Label(right_frame, text="Результат обработки", font=("Arial", 11)).pack()
        self.result_label = tk.Label(right_frame, bg="white", relief=tk.SUNKEN, bd=1)
        self.result_label.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)
        
        # Минимальная информационная панель
        info_frame = tk.Frame(self.root, height=30)
        info_frame.pack(fill=tk.X, padx=5, pady=2)
        
        self.info_label = tk.Label(info_frame, text="Готов к работе. Загрузите изображение.", 
                                  anchor=tk.W, bg="#f0f0f0", relief=tk.SUNKEN, bd=1)
        self.info_label.pack(fill=tk.X)
        
        # Статус бар
        self.status_bar = tk.Label(self.root, text="Готов", bd=1, relief=tk.SUNKEN, anchor=tk.W)
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def create_parameter_panel(self):
        # Очищаем
        for widget in self.param_frame.winfo_children():
            widget.destroy()
        
        processing_type = self.processing_type.get()
        
        if processing_type == "elementwise":
            self.elementwise_op = tk.StringVar(value="add")
            
            # Операции
            op_frame = tk.Frame(self.param_frame)
            op_frame.pack()
            
            ops = [("+", "add"), ("-", "subtract"), ("×", "multiply"), ("log", "log"), ("^", "power")]
            for text, value in ops:
                tk.Radiobutton(op_frame, text=text, variable=self.elementwise_op, 
                              value=value, width=3).pack(side=tk.LEFT, padx=1)
            
            # Значение
            self.elementwise_value = tk.Scale(self.param_frame, from_=0, to=100, 
                                             orient=tk.HORIZONTAL, length=120, showvalue=1)
            self.elementwise_value.set(30)
            self.elementwise_value.pack()
            
        elif processing_type == "contrast":
            # Диапазон
            tk.Label(self.param_frame, text="Диапазон:").pack(anchor=tk.W)
            
            range_frame = tk.Frame(self.param_frame)
            range_frame.pack()
            
            self.contrast_min = tk.Scale(range_frame, from_=0, to=255, 
                                        orient=tk.HORIZONTAL, length=60, showvalue=1)
            self.contrast_min.set(50)
            self.contrast_min.pack(side=tk.LEFT, padx=2)
            
            tk.Label(range_frame, text="-").pack(side=tk.LEFT)
            
            self.contrast_max = tk.Scale(range_frame, from_=0, to=255, 
                                        orient=tk.HORIZONTAL, length=60, showvalue=1)
            self.contrast_max.set(200)
            self.contrast_max.pack(side=tk.LEFT, padx=2)
            
            tk.Button(self.param_frame, text="Авто", command=self.auto_detect_range, 
                     width=6).pack(pady=1)
            
        elif processing_type == "global_threshold":
            self.global_method = tk.StringVar(value="binary")
            
            # Метод
            method_frame = tk.Frame(self.param_frame)
            method_frame.pack()
            
            methods = [("Бин", "binary"), ("Инв", "binary_inv"), ("Усеч", "trunc")]
            for text, value in methods:
                tk.Radiobutton(method_frame, text=text, variable=self.global_method, 
                              value=value).pack(side=tk.LEFT, padx=2)
            
            # Порог
            self.threshold_value = tk.Scale(self.param_frame, from_=0, to=255, 
                                          orient=tk.HORIZONTAL, length=120, showvalue=1)
            self.threshold_value.set(128)
            self.threshold_value.pack()
            
            tk.Button(self.param_frame, text="Отсу", command=self.apply_otsu, 
                     width=6).pack(pady=1)
            
        elif processing_type == "adaptive_threshold":
            self.adaptive_method = tk.StringVar(value="gaussian")
            
            # Метод
            tk.Radiobutton(self.param_frame, text="Ср", variable=self.adaptive_method, 
                          value="mean", width=3).pack(anchor=tk.W)
            tk.Radiobutton(self.param_frame, text="Гаусс", variable=self.adaptive_method, 
                          value="gaussian", width=6).pack(anchor=tk.W)
            
            # Параметры
            param_frame = tk.Frame(self.param_frame)
            param_frame.pack()
            
            tk.Label(param_frame, text="Блок:").pack(side=tk.LEFT)
            self.block_size = tk.Scale(param_frame, from_=3, to=31, 
                                      orient=tk.HORIZONTAL, length=80, showvalue=1)
            self.block_size.set(11)
            self.block_size.pack(side=tk.LEFT, padx=2)
            
            tk.Label(param_frame, text="C:").pack(side=tk.LEFT)
            self.c_value = tk.Scale(param_frame, from_=0, to=30, 
                                   orient=tk.HORIZONTAL, length=60, showvalue=1)
            self.c_value.set(2)
            self.c_value.pack(side=tk.LEFT)

    def update_parameter_panel(self):
        self.create_parameter_panel()

    def load_image(self):
        file_path = filedialog.askopenfilename(
            title="Выберите изображение",
            filetypes=[("Изображения", "*.jpg *.jpeg *.png *.bmp *.tiff")]
        )
        if file_path:
            self.image = cv2.imread(file_path)
            if self.image is None:
                messagebox.showerror("Ошибка", "Не удалось загрузить изображение")
                return
            
            self.gray_image = cv2.cvtColor(self.image, cv2.COLOR_BGR2GRAY)
            self.display_image(self.image, self.original_label)
            self.status_bar.config(text=f"Загружено: {os.path.basename(file_path)}")
            self.auto_detect_range()

    def display_image(self, img, label):
        if img is None:
            return
        
        if len(img.shape) == 3:
            img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        else:
            img_rgb = cv2.cvtColor(img, cv2.COLOR_GRAY2RGB)
        
        img_pil = Image.fromarray(img_rgb)
        
        # Масштабируем по размеру окна с сохранением пропорций
        label_width = label.winfo_width()
        label_height = label.winfo_height()
        
        if label_width > 10 and label_height > 10:
            img_width, img_height = img_pil.size
            scale = min(label_width/img_width, label_height/img_height)
            new_size = (int(img_width*scale), int(img_height*scale))
            img_pil = img_pil.resize(new_size, Image.Resampling.LANCZOS)
        
        img_tk = ImageTk.PhotoImage(img_pil)
        label.configure(image=img_tk)
        label.image = img_tk

    def auto_detect_range(self):
        if self.gray_image is not None:
            min_val = np.percentile(self.gray_image, 5)
            max_val = np.percentile(self.gray_image, 95)
            if hasattr(self, 'contrast_min'):
                self.contrast_min.set(int(min_val))
                self.contrast_max.set(int(max_val))

    def apply_otsu(self):
        if self.gray_image is not None:
            _, thresh = cv2.threshold(self.gray_image, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
            self.display_image(thresh, self.result_label)
            self.update_info("Метод Отсу: автоматический выбор порога")

    def process_image(self):
        if self.image is None:
            messagebox.showwarning("Внимание", "Сначала загрузите изображение")
            return
        
        try:
            processing_type = self.processing_type.get()
            
            if processing_type == "elementwise":
                self.processed_image = self.apply_elementwise()
            elif processing_type == "contrast":
                self.processed_image = self.apply_linear_contrast()
            elif processing_type == "global_threshold":
                self.processed_image = self.apply_global_threshold()
            elif processing_type == "adaptive_threshold":
                self.processed_image = self.apply_adaptive_threshold()
            
            self.display_image(self.processed_image, self.result_label)
            
        except Exception as e:
            messagebox.showerror("Ошибка обработки", str(e))

    def apply_elementwise(self):
        img = self.image.astype(np.float32)
        op_type = self.elementwise_op.get()
        value = self.elementwise_value.get()
        
        if op_type == "add":
            result = cv2.add(img, value)
            info = f"Сложение: +{value}"
        elif op_type == "subtract":
            result = cv2.subtract(img, value)
            info = f"Вычитание: -{value}"
        elif op_type == "multiply":
            result = cv2.multiply(img, value/100.0)
            info = f"Умножение: ×{value/100:.2f}"
        elif op_type == "log":
            c = 255 / np.log(1 + np.max(img))
            result = c * np.log(1 + img)
            info = "Логарифмическое преобразование"
        elif op_type == "power":
            gamma = 2.2
            result = 255 * (img/255) ** (1/gamma)
            info = f"Степенное преобразование (γ={gamma})"
        else:
            result = img
            info = "Исходное изображение"
        
        self.update_info(info)
        return np.clip(result, 0, 255).astype(np.uint8)

    def apply_linear_contrast(self):
        if not hasattr(self, 'contrast_min'):
            return self.image
        
        orig_min = self.contrast_min.get()
        orig_max = self.contrast_max.get()
        
        if orig_min >= orig_max:
            orig_min, orig_max = orig_max - 1, orig_min + 1
        
        if len(self.image.shape) == 3:
            # Для цветного изображения
            channels = []
            for i in range(3):
                channel = self.image[:,:,i]
                channel = np.clip(channel, orig_min, orig_max)
                channel = ((channel - orig_min) / (orig_max - orig_min)) * 255
                channels.append(channel)
            result = np.stack(channels, axis=2).astype(np.uint8)
        else:
            # Для серого изображения
            result = np.clip(self.gray_image, orig_min, orig_max)
            result = ((result - orig_min) / (orig_max - orig_min)) * 255
            result = np.clip(result, 0, 255).astype(np.uint8)
        
        self.update_info(f"Линейное контрастирование: [{orig_min}-{orig_max}]→[0-255]")
        return result

    def apply_global_threshold(self):
        method = self.global_method.get()
        thresh_value = self.threshold_value.get()
        
        method_map = {
            "binary": cv2.THRESH_BINARY,
            "binary_inv": cv2.THRESH_BINARY_INV,
            "trunc": cv2.THRESH_TRUNC
        }
        
        _, result = cv2.threshold(self.gray_image, thresh_value, 255, method_map[method])
        
        method_names = {
            "binary": "Бинаризация",
            "binary_inv": "Инверсная бинаризация",
            "trunc": "Усечение"
        }
        
        self.update_info(f"Глобальная пороговая: {method_names[method]}, порог={thresh_value}")
        return cv2.cvtColor(result, cv2.COLOR_GRAY2BGR)

    def apply_adaptive_threshold(self):
        method = self.adaptive_method.get()
        block_size = self.block_size.get()
        c_value = self.c_value.get()
        
        # Делаем размер блока нечетным
        if block_size % 2 == 0:
            block_size += 1
        
        # Выбираем метод адаптации
        adaptive_method = cv2.ADAPTIVE_THRESH_MEAN_C if method == "mean" else cv2.ADAPTIVE_THRESH_GAUSSIAN_C
        
        # Применяем адаптивную пороговую обработку
        result = cv2.adaptiveThreshold(self.gray_image, 255, adaptive_method, 
                                      cv2.THRESH_BINARY, block_size, c_value)
        
        method_name = "Среднее" if method == "mean" else "Гаусс"
        self.update_info(f"Адаптивная пороговая: {method_name}, блок={block_size}, C={c_value}")
        return cv2.cvtColor(result, cv2.COLOR_GRAY2BGR)

    def update_info(self, text):
        self.info_label.config(text=text)
        self.status_bar.config(text=text)

    def save_image(self):
        if self.processed_image is None:
            messagebox.showwarning("Внимание", "Сначала обработайте изображение")
            return
        
        file_path = filedialog.asksaveasfilename(
            title="Сохранить результат",
            defaultextension=".png",
            filetypes=[("PNG files", "*.png"), ("JPEG files", "*.jpg")]
        )
        if file_path:
            cv2.imwrite(file_path, self.processed_image)
            self.status_bar.config(text=f"Сохранено: {os.path.basename(file_path)}")

def main():
    root = tk.Tk()
    app = ImageProcessor(root)
    root.mainloop()

if __name__ == "__main__":
    main()