using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEditor;

public class TextureBitsExplorer : EditorWindow
{
	public int m_BitStart = 0;
	public int m_BitCount = 8;

	private Texture2D m_Texture;
	private byte[] m_TextureRawData;
	private int m_BlockX, m_BlockY, m_BlockBytes;
	private Texture2D m_Preview;
	private Color32[] m_Pixels;

	[MenuItem("Window/Texture Bits Explorer")]
	public static void Init()
	{
		var w = EditorWindow.GetWindow<TextureBitsExplorer>();
		w.minSize = new Vector2(300,300);
		w.titleContent = new GUIContent("TexBitsExplorer");
		w.Show();
		w.OnSelectionChange();
	}

	public void OnSelectionChange()
	{
		m_Texture = Selection.activeObject as Texture2D;
		UpdateTextureData();
		Repaint();
	}

	public void OnGUI()
	{
		if (m_Texture == null || m_Preview == null)
		{
			GUILayout.Label("No texture selected");
			return;
		}
		GUILayout.Label("Texture: " + m_Texture.name);
		GUILayout.Label(string.Format("{0} x {1} {2}", m_Texture.width, m_Texture.height, m_Texture.format));
		GUILayout.Label(string.Format("Raw size: {0:F1}KB", m_TextureRawData.Length/1024.0f));
		GUILayout.Label(string.Format("Block size: {0}x{1} {2} bytes", m_BlockX, m_BlockY, m_BlockBytes));
		GUILayout.BeginHorizontal();
		GUILayout.Label("Bit Start:");
		m_BitStart = EditorGUILayout.IntSlider(m_BitStart, 0, m_BlockBytes*8-m_BitCount);
		GUILayout.Label("Count:");
		m_BitCount = EditorGUILayout.IntSlider(m_BitCount, 1, Mathf.Min(m_BlockBytes*8,8));
		GUILayout.EndHorizontal();
		GUILayout.BeginHorizontal();
		// https://en.wikipedia.org/wiki/S3_Texture_Compression
		if (m_Texture.format == TextureFormat.DXT5)
		{
			if (GUILayout.Button("alph0")) { m_BitStart = 0; m_BitCount = 8; }
			if (GUILayout.Button("alph1")) { m_BitStart = 8; m_BitCount = 8; }
			if (GUILayout.Button("a0")) { m_BitStart = 16; m_BitCount = 3; }
			if (GUILayout.Button("a1")) { m_BitStart = 19; m_BitCount = 3; }
			if (GUILayout.Button("a2")) { m_BitStart = 22; m_BitCount = 3; }
			if (GUILayout.Button("a3")) { m_BitStart = 25; m_BitCount = 3; }
			if (GUILayout.Button("c0r")) { m_BitStart = 75; m_BitCount = 5; }
			if (GUILayout.Button("c0g")) { m_BitStart = 69; m_BitCount = 6; }
			if (GUILayout.Button("c0b")) { m_BitStart = 64; m_BitCount = 5; }
			if (GUILayout.Button("c1r")) { m_BitStart = 91; m_BitCount = 5; }
			if (GUILayout.Button("c1g")) { m_BitStart = 85; m_BitCount = 6; }
			if (GUILayout.Button("c1b")) { m_BitStart = 80; m_BitCount = 5; }
			if (GUILayout.Button("c0")) { m_BitStart = 96; m_BitCount = 2; }
			if (GUILayout.Button("c1")) { m_BitStart = 98; m_BitCount = 2; }
			if (GUILayout.Button("c2")) { m_BitStart = 100; m_BitCount = 2; }
		}
		if (m_Texture.format == TextureFormat.DXT1)
		{
			if (GUILayout.Button("c0r")) { m_BitStart = 11; m_BitCount = 5; }
			if (GUILayout.Button("c0g")) { m_BitStart = 5; m_BitCount = 6; }
			if (GUILayout.Button("c0b")) { m_BitStart = 0; m_BitCount = 5; }
			if (GUILayout.Button("c1r")) { m_BitStart = 27; m_BitCount = 5; }
			if (GUILayout.Button("c1g")) { m_BitStart = 21; m_BitCount = 6; }
			if (GUILayout.Button("c1b")) { m_BitStart = 16; m_BitCount = 5; }
			if (GUILayout.Button("c0")) { m_BitStart = 32; m_BitCount = 2; }
			if (GUILayout.Button("c1")) { m_BitStart = 34; m_BitCount = 2; }
			if (GUILayout.Button("c2")) { m_BitStart = 36; m_BitCount = 2; }
		}
		// https://github.com/ARM-software/astc-encoder/blob/master/Documentation/ASTC%20Specification%201.0.pdf
		// https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_texture_compression_astc_hdr.txt
		if (m_Texture.format == TextureFormat.ASTC_RGBA_6x6 ||
			m_Texture.format == TextureFormat.ASTC_RGB_6x6 ||
			m_Texture.format == TextureFormat.ASTC_RGBA_4x4 ||
			m_Texture.format == TextureFormat.ASTC_RGB_4x4 ||
			m_Texture.format == TextureFormat.ASTC_RGBA_5x5 ||
			m_Texture.format == TextureFormat.ASTC_RGB_5x5 ||
			m_Texture.format == TextureFormat.ASTC_RGBA_8x8 ||
			m_Texture.format == TextureFormat.ASTC_RGB_8x8)
		{
			// bits 0..10: block mode
			// bits 11..12: number of partitions
			// in single partition case: color endpoint encoding mode is bits 13..16
			if (GUILayout.Button("bm0")) { m_BitStart = 0; m_BitCount = 5; }
			if (GUILayout.Button("bm1")) { m_BitStart = 5; m_BitCount = 4; }
			if (GUILayout.Button("bm2")) { m_BitStart = 9; m_BitCount = 2; }
			if (GUILayout.Button("prt")) { m_BitStart = 11; m_BitCount = 2; }
			if (GUILayout.Button("cem1")) { m_BitStart = 13; m_BitCount = 4; }
		}
		GUILayout.EndHorizontal();
		Rect rect = GUILayoutUtility.GetRect(1, 1, GUILayout.ExpandWidth(false));
		float spaceX = position.width - rect.x;
		float spaceY = position.height - rect.y;
		GUI.DrawTexture(new Rect(rect.x, rect.y, spaceX, spaceY), m_Preview, ScaleMode.ScaleToFit);

		if (GUI.changed)
			UpdatePreviewPixels();
	}

	private void UpdatePreviewPixels()
	{
		int idx = 0;
		int dataIdx = 0;
		int previewWidth = m_Preview.width;
		int previewHeight = m_Preview.height;
		float scale = 255.0f / ((1<<m_BitCount)-1);
		for (int y = 0; y < previewHeight; ++y)
		{
			for (int x = 0; x < previewWidth; ++x)
			{
				Color32 col;
				int value = 0;
				for (int b = 0; b < m_BitCount; ++b)
				{
					if ((m_TextureRawData[dataIdx + (m_BitStart+b)/8] & (1<<((m_BitStart+b) & 7))) != 0)
						value |= 1<<b;
				}
				value = (int)(value * scale);
				col.r = col.g = col.b = (byte)value;

				col.a = 255;
				m_Pixels[idx] = col;
				++idx;
				dataIdx += m_BlockBytes;
			}
		}
		m_Preview.SetPixels32(m_Pixels);
		m_Preview.Apply();
	}

	private void UpdateTextureData()
	{
		DestroyImmediate(m_Preview);
		if (m_Texture == null)
		{
			m_TextureRawData = null;
			m_BlockX = m_BlockY = m_BlockBytes = 1;
			m_Preview = null;
			m_Pixels = null;
			return;
		}
		m_TextureRawData = m_Texture.GetRawTextureData();
		GetTextureFormatDesc(m_Texture.format, out m_BlockX, out m_BlockY, out m_BlockBytes);

		int previewWidth = (m_Texture.width + m_BlockX-1) / m_BlockX;
		int previewHeight = (m_Texture.height + m_BlockY-1) / m_BlockY;
		m_Preview = new Texture2D(previewWidth, previewHeight, TextureFormat.RGBA32, false);
		m_Preview.filterMode = FilterMode.Point;
		m_Pixels = new Color32[previewWidth*previewHeight];
		UpdatePreviewPixels();
		SaveTextureRawBits();
	}

	private void GetTextureFormatDesc(TextureFormat format, out int sizeX, out int sizeY, out int bytesPerBlock)
	{
		switch(format)
		{
		case TextureFormat.DXT5:
			sizeX = sizeY = 4;
			bytesPerBlock = 16;
			break;
		case TextureFormat.DXT1:
			sizeX = sizeY = 4;
			bytesPerBlock = 8;
			break;
		case TextureFormat.ASTC_RGBA_4x4:
		case TextureFormat.ASTC_RGB_4x4:
			sizeX = sizeY = 4;
			bytesPerBlock = 16;
			break;
		case TextureFormat.ASTC_RGBA_5x5:
		case TextureFormat.ASTC_RGB_5x5:
			sizeX = sizeY = 5;
			bytesPerBlock = 16;
			break;
		case TextureFormat.ASTC_RGBA_6x6:
		case TextureFormat.ASTC_RGB_6x6:
			sizeX = sizeY = 6;
			bytesPerBlock = 16;
			break;
		case TextureFormat.ASTC_RGBA_8x8:
		case TextureFormat.ASTC_RGB_8x8:
			sizeX = sizeY = 8;
			bytesPerBlock = 16;
			break;
		case TextureFormat.ASTC_RGBA_10x10:
		case TextureFormat.ASTC_RGB_10x10:
			sizeX = sizeY = 10;
			bytesPerBlock = 16;
			break;
		case TextureFormat.ASTC_RGBA_12x12:
		case TextureFormat.ASTC_RGB_12x12:
			sizeX = sizeY = 12;
			bytesPerBlock = 16;
			break;
		case TextureFormat.ARGB32:
		case TextureFormat.RGBA32:
			sizeX = sizeY = 1;
			bytesPerBlock = 4;
			break;
		default:
			sizeX = sizeY = 1;
			bytesPerBlock = 1;
			break;
		}
	}

	void SaveTextureRawBits()
	{
		var folder = string.Format("bits/{0}", m_Texture.format.ToString().ToLower());
		Directory.CreateDirectory(folder);
		int toSave = m_Preview.width * m_Preview.height * m_BlockBytes;
		var stream = new FileStream(string.Format("{0}/{1}-{2}x{3}.bin", folder, m_Texture.name, m_Texture.width, m_Texture.height), FileMode.Create, FileAccess.Write);
		stream.Write(m_TextureRawData, 0, toSave);
		stream.Close();
	}
}
