using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;
using UnityEngine.UI;

public class ModeSet : MonoBehaviour {

    public GUIStyle style, informStyle;

    private Process p;
    private bool initiated = false;
    private int mode = -1;
    private int inform = 0;
	// Use this for initialization
	void Start ()
    {
        p = new Process();
        p.StartInfo.FileName = (System.AppDomain.CurrentDomain.SetupInformation.ApplicationBase + "EyeX.exe");
        p.StartInfo.CreateNoWindow = true;
        p.StartInfo.UseShellExecute = false;
        p.StartInfo.RedirectStandardOutput = true; 
    }

   
	
	// Update is called once per frame
	void Update ()
    {
        if (initiated && p.HasExited)
        {
            if (GameObject.Find("Reading").GetComponentInChildren<Text>().text == "阅读模式：开启")
            {
                GameObject.Find("Reading").GetComponentInChildren<Text>().text = "阅读模式：关闭";
                mode = -1;
            }
            if (GameObject.Find("Assistancy").GetComponentInChildren<Text>().text == "辅助模式：开启")
            {
                GameObject.Find("Assistancy").GetComponentInChildren<Text>().text = "辅助模式：关闭";
                mode = -1;
            }
        }

    }

    public void ReadingModeSet()
    {
        if (initiated && !p.HasExited)
        {
            p.Kill();
            if (mode >= 0)
            {
                mode = -1;
                return;
            }
        }
        p.StartInfo.Arguments = "1";
        p.Start();
        initiated = true;
        GameObject.Find("Reading").GetComponentInChildren<Text>().text = "阅读模式：开启";
        mode = 0;
    }
    public void AssistancyModeSet()
    {

        if (initiated && !p.HasExited)
            p.Kill();

        p.StartInfo.Arguments = mode == 1 ? "1" : "3";
        p.Start();
        initiated = true;
        GameObject.Find("Reading").GetComponentInChildren<Text>().text = "阅读模式：开启";
        mode = mode == 1 ? 0 : 1;
        GameObject.Find("Assistancy").GetComponentInChildren<Text>().text = mode == 1 ? "辅助模式：开启" : "辅助模式：关闭";
    }

    void OnGUI()
    {
        if (GUI.Button(new Rect(580, 40, 16, 16), "", style))
            inform = 1 - inform;
        if (inform == 1)
            GUI.Label(new Rect(420, 10, 150, 105), "制作人员：\n陈俊    897162163\n李强    1135013119\n梅俊祥 326738188", informStyle);
    }
}
